#include "fd_sim.h"
#include "fd_biology.h"
#include "fd_chemistry.h"
#include "fd_geometry.h"
#include "fd_nutrition.h"
#include "fd_physics.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void push_history(fd_sim_state_t *sim) {
    int idx = sim->hist_head;
    sim->hist_time[idx]     = (float)sim->elapsed_sec;
    sim->hist_t_surf[idx]   = sim->metrics.surface_max_temp_c;
    sim->hist_t_cold[idx]   = sim->metrics.cold_spot_temp_c;
    sim->hist_maillard[idx] = sim->metrics.crust_maillard;
    sim->hist_moisture[idx] = sim->metrics.avg_moisture;
    sim->hist_head = (sim->hist_head + 1) % FD_HISTORY_LEN;
    if (sim->hist_count < FD_HISTORY_LEN) sim->hist_count++;
    sim->last_hist_sec = sim->elapsed_sec;
}

static void refresh_derived(fd_sim_state_t *sim, const fd_database_t *db) {
    if (!sim || !sim->initialized || !db) return;
    float base_ph = (sim->primary_ing_idx >= 0 && sim->primary_ing_idx < db->ingredient_count)
                  ? db->ingredients[sim->primary_ing_idx].ph : 6.1f;
    float aq_mass_g = fmaxf(8.0f, sim->total_water_g + 0.35f * sim->dry_protein_g);
    sim->metrics.ph = fd_compute_ph(sim->acid_mol, sim->base_mol, aq_mass_g, base_ph);
    fd_biology_step(sim, db, 0.0f);
    fd_nutrition_update(sim);
}

static float grid_avg_temp(const fd_grid_t *g) {
    if (!g || !g->T || g->count <= 0) return 20.0f;
    float sum = 0.0f;
    int n = 0;
    for (int i = 0; i < g->count; i++) {
        if (g->tissue[i] != FD_TISSUE_EMPTY) { sum += g->T[i]; n++; }
    }
    return (n > 0) ? (sum / (float)n) : 20.0f;
}

static float sim_specific_heat_j_g_k(const fd_sim_state_t *sim) {
    float w = fmaxf(1.0f, sim->raw_weight_g);
    return clampf(4.18f * (sim->total_water_g / w) + 2.0f * ((w - sim->total_water_g) / w), 1.6f, 4.18f);
}

void fd_sim_free(fd_sim_state_t *sim) {
    if (!sim) return;
    fd_grid_free(&sim->grid);
    memset(sim, 0, sizeof(*sim));
}

bool fd_sim_init_dish(fd_sim_state_t *sim, const fd_database_t *db,
                      const fd_recipe_item_t *items, int item_count,
                      fd_shape_type_t shape, float lx_mm, float ly_mm, float lz_mm,
                      float init_temp_c, bool coarse_mode) {
    if (!sim || !db || !items || item_count <= 0) return false;
    char saved_name[48];
    snprintf(saved_name, sizeof(saved_name), "%s", sim->vessel_name);
    fd_vessel_kind_t saved_kind = sim->vessel_kind;
    fd_sim_free(sim);
    snprintf(sim->vessel_name, sizeof(sim->vessel_name), "%s", saved_name[0] ? saved_name : "Primary Vessel");
    sim->vessel_kind = saved_kind;
    sim->shape       = shape;
    sim->burner_on   = true;

    sim->smoke_point_min_c = 999.0f;
    sim->d_ref_sec = 32.0f;
    sim->t_ref_c   = 60.0f;
    sim->z_val_c   = 5.5f;
    sim->primary_ing_idx = -1;

    float max_solid_mass = -1.0f;
    float weighted_aniso = 1.0f;

    for (int i = 0; i < item_count && sim->item_count < FD_MAX_RECIPE_ITEMS; i++) {
        int idx = fd_db_find_ingredient(db, items[i].ingredient_id);
        float m = items[i].mass_g;
        if (idx < 0 || m <= 0.01f) continue;

        const fd_ingredient_t *ing = &db->ingredients[idx];
        int slot = sim->item_count++;
        sim->ing_indices[slot]  = idx;
        sim->ing_masses_g[slot] = m;

        sim->raw_weight_g         += m;
        sim->total_water_g        += m * ing->water;
        sim->dry_protein_g        += m * ing->protein;
        sim->dry_collagen_g       += m * ing->protein * ing->collagen_frac;
        sim->total_fat_g          += m * ing->fat;
        sim->dry_starch_g         += m * ing->starch;
        sim->dry_sugar_g          += m * ing->sugar;
        sim->dry_reducing_sugar_g += m * ing->sugar * ing->reducing_sugar_frac;
        sim->dry_fiber_g          += m * ing->fiber;
        sim->dry_ash_g            += m * ing->ash;

        /* NaCl equivalent from sodium_mg_100g (Na is 39.34% of NaCl) */
        sim->salt_g           += (m * 0.01f) * (ing->sodium_mg_100g * 0.001f / 0.3934f);
        sim->acid_mol         += (m * 0.001f) * ing->acidity_molar;
        if (ing->ph > 7.2f) {
            sim->base_mol += (m * 0.001f) * (ing->ph - 7.0f) * 0.12f;
        }
        sim->glutamate_mg     += (m * 0.01f) * ing->glutamate_mg_100g;
        sim->nucleotides_mg   += (m * 0.01f) * ing->nucleotides_mg_100g;
        sim->emulsifier_units += m * ing->emulsifier_power;
        sim->free_amino_pool  += m * ing->free_amino;
        sim->aroma_pool       += m * ing->aroma_potency;
        sim->bitter_pool      += m * ing->bitter_base;
        sim->pungency_pool    += m * ing->pungency;
        sim->antinutrient_pool+= m * ing->antinutrient_frac;

        if (ing->yeast_activity > sim->yeast_viability)     sim->yeast_viability   = ing->yeast_activity;
        if (ing->lacto_activity > sim->lacto_viability)     sim->lacto_viability   = ing->lacto_activity;
        if (ing->amylase_activity > sim->amylase_viability) sim->amylase_viability = ing->amylase_activity;

        if (ing->fat > 0.15f && ing->smoke_point_c < sim->smoke_point_min_c) {
            sim->smoke_point_min_c = ing->smoke_point_c;
        }
        if (ing->requires_pasteurization) {
            if (!sim->needs_pasteurization || ing->d_ref_sec > sim->d_ref_sec) {
                sim->d_ref_sec = ing->d_ref_sec;
                sim->t_ref_c   = ing->t_ref_c;
                sim->z_val_c   = ing->z_val_c;
            }
            sim->needs_pasteurization = true;
        }
        if (m > max_solid_mass && !(ing->role_flags & FD_ROLE_SALT)) {
            max_solid_mass = m;
            sim->primary_ing_idx = idx;
            weighted_aniso = ing->fiber_anisotropy;
        }
    }

    if (sim->item_count == 0 || sim->raw_weight_g <= 0.01f) return false;

    float w_inv = 1.0f / sim->raw_weight_g;
    sim->free_amino_pool   *= w_inv;
    sim->aroma_pool        *= w_inv;
    sim->bitter_pool       *= w_inv;
    sim->pungency_pool     *= w_inv;
    sim->antinutrient_pool *= w_inv;

    /* Hydrated gluten-bearing flour starts with baseline autolyse network (kneading develops -> 0.98) */
    if (sim->dry_starch_g * w_inv > 0.25f && sim->dry_protein_g * w_inv > 0.04f && sim->primary_ing_idx >= 0 &&
        !(db->ingredients[sim->primary_ing_idx].diet_flags & FD_DIET_GLUTEN_FREE)) {
        sim->gluten_network = 0.42f;
    }

    int nx = 1, ny = 1, nz = 1;
    fd_geometry_choose_dims(shape, lx_mm, ly_mm, lz_mm, coarse_mode, &nx, &ny, &nz);
    if (!fd_grid_alloc(&sim->grid, nx, ny, nz, lx_mm, ly_mm, lz_mm)) return false;

    fd_grid_rasterize(&sim->grid, shape, init_temp_c,
                      fminf(0.94f, sim->total_water_g * w_inv),
                      fminf(0.85f, sim->total_fat_g * w_inv),
                      fminf(0.12f, sim->salt_g * w_inv),
                      weighted_aniso);

    sim->ambient_temp_c = init_temp_c;
    sim->initialized = true;
    sim->metrics.emulsion_stability = 1.0f;
    refresh_derived(sim, db);
    push_history(sim);
    return true;
}

bool fd_sim_add_item(fd_sim_state_t *sim, const fd_database_t *db,
                     const fd_recipe_item_t *item, fd_shape_type_t fallback_shape,
                     float lx_mm, float ly_mm, float lz_mm,
                     float item_temp_c, bool coarse_mode) {
    if (!sim || !db || !item || item->mass_g <= 0.01f) return false;
    if (!sim->initialized || sim->consumed || !sim->grid.T) {
        float saved_fond = sim->fond_maillard;
        const fd_ingredient_t *ing0 = NULL;
        int idx0 = fd_db_find_ingredient(db, item->ingredient_id);
        if (idx0 >= 0) ing0 = &db->ingredients[idx0];
        fd_shape_type_t sh = (ing0 && ing0->default_shape == FD_SHAPE_LIQUID) ? FD_SHAPE_LIQUID : fallback_shape;
        bool ok = fd_sim_init_dish(sim, db, item, 1, sh, lx_mm, ly_mm, lz_mm, item_temp_c, coarse_mode);
        if (ok && saved_fond > 0.0f) sim->fond_maillard = saved_fond;
        return ok;
    }
    int idx = fd_db_find_ingredient(db, item->ingredient_id);
    if (idx < 0) return false;
    const fd_ingredient_t *ing = &db->ingredients[idx];
    float m = item->mass_g;

    float old_w = fmaxf(0.1f, sim->raw_weight_g);
    float new_w = old_w + m;
    float old_cap = fmaxf(1.0f, sim->metrics.cooked_weight_g) * sim_specific_heat_j_g_k(sim);
    float add_cp  = 4.18f * ing->water + 2.0f * (1.0f - ing->water);
    float add_cap = m * add_cp;

    if (sim->item_count < FD_MAX_RECIPE_ITEMS) {
        int slot = sim->item_count++;
        sim->ing_indices[slot]  = idx;
        sim->ing_masses_g[slot] = m;
    }

    sim->free_amino_pool   = (sim->free_amino_pool * old_w + m * ing->free_amino) / new_w;
    sim->aroma_pool        = (sim->aroma_pool * old_w + m * ing->aroma_potency) / new_w;
    sim->bitter_pool       = (sim->bitter_pool * old_w + m * ing->bitter_base) / new_w;
    sim->pungency_pool     = (sim->pungency_pool * old_w + m * ing->pungency) / new_w;
    sim->antinutrient_pool = (sim->antinutrient_pool * old_w + m * ing->antinutrient_frac) / new_w;

    sim->raw_weight_g         = new_w;
    sim->total_water_g        += m * ing->water;
    sim->dry_protein_g        += m * ing->protein;
    sim->dry_collagen_g       += m * ing->protein * ing->collagen_frac;
    sim->total_fat_g          += m * ing->fat;
    sim->dry_starch_g         += m * ing->starch;
    sim->dry_sugar_g          += m * ing->sugar;
    sim->dry_reducing_sugar_g += m * ing->sugar * ing->reducing_sugar_frac;
    sim->dry_fiber_g          += m * ing->fiber;
    sim->dry_ash_g            += m * ing->ash;
    sim->salt_g               += (m * 0.01f) * (ing->sodium_mg_100g * 0.001f / 0.3934f);
    sim->acid_mol             += (m * 0.001f) * ing->acidity_molar;
    if (ing->ph > 7.2f) sim->base_mol += (m * 0.001f) * (ing->ph - 7.0f) * 0.12f;
    sim->glutamate_mg         += (m * 0.01f) * ing->glutamate_mg_100g;
    sim->nucleotides_mg       += (m * 0.01f) * ing->nucleotides_mg_100g;
    sim->emulsifier_units     += m * ing->emulsifier_power;

    if (ing->yeast_activity > sim->yeast_viability)     sim->yeast_viability   = ing->yeast_activity;
    if (ing->lacto_activity > sim->lacto_viability)     sim->lacto_viability   = ing->lacto_activity;
    if (ing->amylase_activity > sim->amylase_viability) sim->amylase_viability = ing->amylase_activity;
    if (ing->fat > 0.15f && ing->smoke_point_c < sim->smoke_point_min_c) {
        sim->smoke_point_min_c = ing->smoke_point_c;
    }
    if (ing->requires_pasteurization) {
        if (!sim->needs_pasteurization || ing->d_ref_sec > sim->d_ref_sec) {
            sim->d_ref_sec = ing->d_ref_sec;
            sim->t_ref_c   = ing->t_ref_c;
            sim->z_val_c   = ing->z_val_c;
        }
        sim->needs_pasteurization = true;
        for (int i = 0; i < sim->grid.count; i++) {
            if (sim->grid.tissue[i] != FD_TISSUE_EMPTY) sim->grid.lethality_log10[i] = 0.0f;
        }
    }

    /* Enthalpy-conserving thermal equilibration onto vessel liquid or exposed solid surface */
    float cur_t = grid_avg_temp(&sim->grid);
    float dt_mix = (add_cap / fmaxf(1.0f, old_cap + add_cap)) * (item_temp_c - cur_t);
    for (int i = 0; i < sim->grid.count; i++) {
        if (sim->grid.tissue[i] == FD_TISSUE_EMPTY) continue;
        if (sim->grid.count == 1 || sim->grid.surface_mask[i] != 0) {
            sim->grid.T[i] += dt_mix;
        }
    }
    refresh_derived(sim, db);
    return true;
}

bool fd_sim_set_technique(fd_sim_state_t *sim, const fd_database_t *db,
                          const char *tech_id, float temp_c, bool lid_closed) {
    if (!sim || !db) return false;
    int idx = fd_db_find_technique(db, tech_id);
    if (idx < 0) {
        if (db->technique_count == 0) return false;
        idx = 0;
    }
    sim->active_tech_idx = idx;
    const fd_technique_t *t = &db->techniques[idx];
    if (temp_c < t->min_temp_c) temp_c = t->min_temp_c;
    if (temp_c > t->max_temp_c) temp_c = t->max_temp_c;
    sim->ambient_temp_c = temp_c;
    sim->lid_closed     = lid_closed;
    sim->burner_on      = true;
    return true;
}

void fd_sim_step(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua, float dt_sec) {
    if (!sim || !sim->initialized || sim->consumed || !db || dt_sec <= 0.0f) return;

    fd_physics_step(sim, db, dt_sec);
    fd_chemistry_step(sim, db, dt_sec);
    fd_biology_step(sim, db, dt_sec);
    fd_nutrition_update(sim);
    sim->elapsed_sec += (double)dt_sec;

    if (lua && lua->mod_count > 0) {
        fd_lua_call_on_step(lua, sim->elapsed_sec, sim->metrics.core_temp_c, sim->metrics.surface_max_temp_c);
        float bonus = fd_lua_call_on_score(lua, &sim->metrics);
        float adj = sim->metrics.overall_score + bonus;
        sim->metrics.overall_score = adj < 0.0f ? 0.0f : (adj > 100.0f ? 100.0f : adj);
    }

    if (sim->elapsed_sec - sim->last_hist_sec >= 2.0) {
        push_history(sim);
    }
}

static void step_chunked(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua,
                         float dur, bool flip_midway, bool coarse_mode) {
    dur = fmaxf(1.0f, dur);
    float chunk = coarse_mode ? fminf(30.0f, fmaxf(4.0f, dur / 12.0f))
                              : fminf(15.0f, fmaxf(2.0f, dur / 36.0f));
    float elapsed = 0.0f;
    bool flipped = false;
    while (elapsed < dur - 1e-3f) {
        float dt = fminf(chunk, dur - elapsed);
        if (flip_midway && !flipped && elapsed >= dur * 0.5f) {
            if (sim->shape == FD_SHAPE_CUBE) fd_grid_toss_cubes(&sim->grid);
            else fd_grid_flip_z(&sim->grid);
            flipped = true;
        }
        fd_sim_step(sim, db, lua, dt);
        elapsed += dt;
    }
}

bool fd_sim_apply_action(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua,
                         fd_step_action_t action, float agitation,
                         float cut_lx_mm, float cut_ly_mm, float cut_lz_mm,
                         fd_shape_type_t cut_shape, bool has_cut_shape,
                         float temp_c, float duration_sec, bool coarse_mode) {
    if (!sim || !sim->initialized || !db) return false;
    float ag = (agitation > 0.01f) ? clampf(agitation, 0.05f, 1.0f) : 0.70f;
    float dur = (duration_sec > 0.5f) ? duration_sec : 30.0f;

    switch (action) {
        case FD_ACT_CUT_PREP: {
            float cur_lx = sim->grid.lx * 1000.0f;
            float cur_ly = sim->grid.ly * 1000.0f;
            float cur_lz = sim->grid.lz * 1000.0f;
            float nlx = (cut_lx_mm > 1.0f) ? cut_lx_mm : fmaxf(4.0f, cur_lx * 0.5f);
            float nly = (cut_ly_mm > 1.0f) ? cut_ly_mm : fmaxf(4.0f, cur_ly * 0.5f);
            float nlz = (cut_lz_mm > 1.0f) ? cut_lz_mm : fmaxf(4.0f, cur_lz * 0.5f);
            fd_shape_type_t nsh = has_cut_shape ? cut_shape
                                : ((sim->shape == FD_SHAPE_LIQUID) ? FD_SHAPE_LIQUID : FD_SHAPE_CUBE);

            /* Slicing hot un-rested muscle fibers (rare through well-done) causes immediate juice purge */
            if (sim->metrics.surface_max_temp_c > 75.0f && sim->metrics.core_temp_c > 42.0f &&
                (sim->dry_protein_g / fmaxf(1.0f, sim->raw_weight_g)) > 0.12f &&
                sim->rested_frac < 0.45f) {
                float purge = 0.055f * (1.0f - sim->rested_frac);
                for (int i = 0; i < sim->grid.count; i++) {
                    if (sim->grid.tissue[i] != FD_TISSUE_EMPTY) {
                        sim->grid.W[i] = fmaxf(0.15f, sim->grid.W[i] - purge);
                    }
                }
                sim->total_water_g = fmaxf(2.0f, sim->total_water_g - purge * sim->raw_weight_g);
            }

            if (sim->shape != FD_SHAPE_LIQUID) {
                fd_grid_resize_preserve(&sim->grid, nsh, nlx, nly, nlz, coarse_mode);
                sim->shape = nsh;
            }
            float d_min = fminf(nlx, fminf(nly, nlz));
            sim->cell_rupture = clampf(fmaxf(sim->cell_rupture, 1.0f - expf(-9.5f / fmaxf(1.5f, d_min))), 0.0f, 0.98f);
            refresh_derived(sim, db);
            push_history(sim);
            return true;
        }
        case FD_ACT_POUND: {
            float cur_lx = sim->grid.lx * 1000.0f;
            float cur_ly = sim->grid.ly * 1000.0f;
            float cur_lz = sim->grid.lz * 1000.0f;
            float nlz = (cut_lz_mm > 1.0f) ? cut_lz_mm : fmaxf(5.0f, cur_lz * 0.45f);
            float expand = sqrtf(fmaxf(1.0f, cur_lz / nlz));
            float nlx = (cut_lx_mm > 1.0f) ? cut_lx_mm : cur_lx * expand;
            float nly = (cut_ly_mm > 1.0f) ? cut_ly_mm : cur_ly * expand;
            fd_shape_type_t nsh = has_cut_shape ? cut_shape : FD_SHAPE_SLAB;
            if (sim->shape != FD_SHAPE_LIQUID) {
                fd_grid_resize_preserve(&sim->grid, nsh, nlx, nly, nlz, coarse_mode);
                sim->shape = nsh;
            }
            sim->mech_tender = clampf(sim->mech_tender + 0.55f, 0.0f, 1.0f);
            refresh_derived(sim, db);
            push_history(sim);
            return true;
        }
        case FD_ACT_SALT_BRINE: {
            sim->mech_tender = clampf(sim->mech_tender + 0.30f, 0.0f, 1.0f);
            fd_sim_set_technique(sim, db, "cure_marinade", (temp_c > 0.0f) ? temp_c : 18.0f, true);
            step_chunked(sim, db, lua, dur, false, coarse_mode);
            return true;
        }
        case FD_ACT_WHISK: {
            if (fd_db_find_technique(db, "whisk_whip") >= 0 && temp_c <= 32.0f) {
                fd_sim_set_technique(sim, db, "whisk_whip", (temp_c > 0.0f) ? temp_c : 22.0f, false);
            } else {
                fd_sim_set_technique(sim, db, "whisk_emulsify", (temp_c > 0.0f) ? temp_c : sim->ambient_temp_c, false);
            }
            sim->shear_boost = 55.0f + 135.0f * ag;
            step_chunked(sim, db, lua, dur, false, coarse_mode);
            sim->shear_boost = 0.0f;
            return true;
        }
        case FD_ACT_KNEAD: {
            if (sim->dry_starch_g > 5.0f && sim->dry_protein_g > 1.5f) {
                float gain = (1.0f - sim->gluten_network) * (1.0f - expf(-0.0065f * ag * fmaxf(60.0f, dur)));
                sim->gluten_network = clampf(sim->gluten_network + gain, 0.0f, 0.98f);
            }
            fd_sim_set_technique(sim, db, "ferment_proof", (temp_c > 0.0f) ? temp_c : 25.0f, true);
            step_chunked(sim, db, lua, fminf(dur, 180.0f), false, coarse_mode);
            return true;
        }
        case FD_ACT_FOLD_STIR: {
            if (agitation > 0.45f) {
                /* Vigorous stirring knocks out delicate whipped foam bubbles */
                sim->foam_air *= 0.56f;
                for (int i = 0; i < sim->grid.count; i++) {
                    if (sim->grid.tissue[i] != FD_TISSUE_EMPTY) sim->grid.porosity[i] *= 0.60f;
                }
                if (sim->grid.count > 1) fd_grid_toss_cubes(&sim->grid);
            } else {
                sim->foam_air *= 0.92f;
                for (int i = 0; i < sim->grid.count; i++) {
                    if (sim->grid.tissue[i] != FD_TISSUE_EMPTY) sim->grid.porosity[i] *= 0.93f;
                }
            }
            sim->emulsion_shear_integral += (30.0f + 80.0f * ag) * fminf(dur, 60.0f);
            refresh_derived(sim, db);
            push_history(sim);
            return true;
        }
        case FD_ACT_DEGLAZE: {
            if (sim->fond_maillard > 0.01f) {
                sim->glutamate_mg += 75.0f * sim->fond_maillard;
                sim->aroma_pool = clampf(sim->aroma_pool + 0.28f * sim->fond_maillard, 0.0f, 1.8f);
                for (int i = 0; i < sim->grid.count; i++) {
                    if (sim->grid.tissue[i] != FD_TISSUE_EMPTY) {
                        sim->grid.maillard[i] = clampf(fmaxf(sim->grid.maillard[i], 0.65f * sim->fond_maillard), 0.0f, 1.0f);
                    }
                }
            }
            fd_sim_set_technique(sim, db, "simmer_poach", (temp_c > 40.0f) ? temp_c : 92.0f, false);
            step_chunked(sim, db, lua, dur, false, coarse_mode);
            return true;
        }
        case FD_ACT_REST: {
            const char *rt = (fd_db_find_technique(db, "rest_hold") >= 0) ? "rest_hold" : "ferment_proof";
            fd_sim_set_technique(sim, db, rt, (temp_c > 0.0f && temp_c <= 55.0f) ? temp_c : 26.0f, false);
            sim->burner_on = false;
            step_chunked(sim, db, lua, dur, false, coarse_mode);
            return true;
        }
        case FD_ACT_COOK:
        case FD_ACT_COMBINE:
        default:
            step_chunked(sim, db, lua, dur, false, coarse_mode);
            return true;
    }
}

bool fd_sim_combine_vessels(fd_sim_state_t *dst, fd_sim_state_t *src,
                            const fd_database_t *db, fd_lua_t *lua,
                            bool gentle_fold, bool coarse_mode) {
    (void)lua;
    if (!dst || !src || dst == src || !src->initialized || src->consumed || !db) return false;

    /* If destination vessel is still empty, move source state into destination (leaving pan fond behind) */
    if (!dst->initialized || !dst->grid.T) {
        char saved_name[48];
        snprintf(saved_name, sizeof(saved_name), "%s", dst->vessel_name);
        fd_vessel_kind_t saved_kind = dst->vessel_kind;
        float pan_fond = src->fond_maillard;
        *dst = *src;
        snprintf(dst->vessel_name, sizeof(dst->vessel_name), "%s", saved_name[0] ? saved_name : src->vessel_name);
        dst->vessel_kind = saved_kind;
        memset(&src->grid, 0, sizeof(src->grid));
        src->initialized   = false;
        src->consumed      = true;
        src->burner_on     = false;
        src->fond_maillard = pan_fond;
        return true;
    }

    float w_d = fmaxf(0.1f, dst->raw_weight_g);
    float w_s = fmaxf(0.1f, src->raw_weight_g);
    float w_tot = w_d + w_s;

    /* Adopt src's 3D voxel grid if dst is liquid OR src is the primary solid food entering oil/seasoning */
    if (src->grid.count > 1 && (dst->grid.count == 1 || w_s > w_d * 1.5f)) {
        fd_grid_t tmp_g = dst->grid;
        dst->grid  = src->grid;
        src->grid  = tmp_g;
        dst->shape = src->shape;
    } else if (src->grid.count == 1 && dst->grid.count > 1 && w_s > w_d * 1.5f) {
        /* Pouring heavier liquid batter/custard into a dry-dredged or sugared baking mold (e.g. fondant ramekin) */
        float w_mix_frac = clampf((dst->total_water_g + src->total_water_g) / w_tot, 0.05f, 0.92f);
        float f_mix_frac = clampf((dst->total_fat_g + src->total_fat_g) / w_tot, 0.0f, 0.80f);
        for (int i = 0; i < dst->grid.count; i++) {
            if (dst->grid.tissue[i] == FD_TISSUE_EMPTY) continue;
            dst->grid.W[i]   = w_mix_frac;
            dst->grid.fat[i] = f_mix_frac;
            dst->grid.T[i]   = (dst->grid.T[i] * w_d + src->grid.T[0] * w_s) / w_tot;
        }
    }

    /* Enthalpy-conserving equilibrium mix temperature */
    float t_dst = grid_avg_temp(&dst->grid);
    float t_src = grid_avg_temp(&src->grid);
    float c_dst = fmaxf(1.0f, dst->metrics.cooked_weight_g) * sim_specific_heat_j_g_k(dst);
    float c_src = fmaxf(1.0f, src->metrics.cooked_weight_g) * sim_specific_heat_j_g_k(src);
    float t_mix = (c_dst * t_dst + c_src * t_src) / (c_dst + c_src);
    float src_frac = c_src / (c_dst + c_src);

    /* Merge ingredient manifest slots */
    for (int i = 0; i < src->item_count && dst->item_count < FD_MAX_RECIPE_ITEMS; i++) {
        dst->ing_indices[dst->item_count]  = src->ing_indices[i];
        dst->ing_masses_g[dst->item_count] = src->ing_masses_g[i];
        dst->item_count++;
    }

    dst->free_amino_pool   = (dst->free_amino_pool * w_d + src->free_amino_pool * w_s) / w_tot;
    dst->aroma_pool        = (dst->aroma_pool * w_d + src->aroma_pool * w_s) / w_tot;
    dst->bitter_pool       = (dst->bitter_pool * w_d + src->bitter_pool * w_s) / w_tot;
    dst->pungency_pool     = (dst->pungency_pool * w_d + src->pungency_pool * w_s) / w_tot;
    dst->antinutrient_pool = (dst->antinutrient_pool * w_d + src->antinutrient_pool * w_s) / w_tot;

    dst->raw_weight_g         = w_tot;
    dst->dry_protein_g        += src->dry_protein_g;
    dst->dry_collagen_g       += src->dry_collagen_g;
    dst->dry_starch_g         += src->dry_starch_g;
    dst->dry_sugar_g          += src->dry_sugar_g;
    dst->dry_reducing_sugar_g += src->dry_reducing_sugar_g;
    dst->dry_fiber_g          += src->dry_fiber_g;
    dst->dry_ash_g            += src->dry_ash_g;
    dst->total_water_g        += src->total_water_g;
    dst->total_fat_g          += src->total_fat_g;
    dst->absorbed_oil_g       += src->absorbed_oil_g;
    dst->salt_g               += src->salt_g;
    dst->acid_mol             += src->acid_mol;
    dst->base_mol             += src->base_mol;
    dst->glutamate_mg         += src->glutamate_mg + 60.0f * src->fond_maillard;
    dst->nucleotides_mg       += src->nucleotides_mg;
    dst->emulsifier_units     += src->emulsifier_units;

    if (src->smoke_point_min_c < dst->smoke_point_min_c) dst->smoke_point_min_c = src->smoke_point_min_c;
    if (src->metrics.oil_smoked) dst->metrics.oil_smoked = true;
    dst->cell_rupture            = fmaxf(dst->cell_rupture, src->cell_rupture);
    dst->gluten_network          = fmaxf(dst->gluten_network, src->gluten_network);
    dst->mech_tender             = fmaxf(dst->mech_tender, src->mech_tender);
    dst->rested_frac             = fmaxf(dst->rested_frac, src->rested_frac);
    dst->fond_maillard           = fmaxf(dst->fond_maillard, src->fond_maillard);
    dst->emulsion_shear_integral = fmaxf(dst->emulsion_shear_integral, src->emulsion_shear_integral) + 180.0f;

    float foam_keep = gentle_fold ? 0.90f : 0.52f;
    float merged_foam = foam_keep * fmaxf(dst->foam_air, src->foam_air);
    dst->foam_air = merged_foam;

    /* Worst-case microbial safety across combined components (e.g. tempering raw egg yolks) */
    bool dst_already_safe = (!dst->needs_pasteurization) || dst->metrics.is_pasteurized_safe;
    bool src_unpast = src->needs_pasteurization && !src->metrics.is_pasteurized_safe;
    if (src->needs_pasteurization) {
        if (dst_already_safe || !dst->needs_pasteurization || src->d_ref_sec > dst->d_ref_sec) {
            dst->d_ref_sec = src->d_ref_sec;
            dst->t_ref_c   = src->t_ref_c;
            dst->z_val_c   = src->z_val_c;
        }
        dst->needs_pasteurization = true;
    }

    for (int i = 0; i < dst->grid.count; i++) {
        if (dst->grid.tissue[i] == FD_TISSUE_EMPTY) continue;
        if (dst->grid.count == 1) {
            dst->grid.T[i]          = t_mix;
            dst->grid.maillard[i]   = (dst->grid.maillard[i] * w_d + src->metrics.crust_maillard * w_s) / w_tot;
            dst->grid.caramel[i]    = (dst->grid.caramel[i] * w_d + src->metrics.crust_caramel * w_s) / w_tot;
            dst->grid.starch_gel[i] = fmaxf(dst->grid.starch_gel[i], src->metrics.starch_gelatinized);
            dst->grid.gelatin[i]    = fmaxf(dst->grid.gelatin[i], src->metrics.gelatin_converted);
        } else if (dst->grid.surface_mask[i] != 0) {
            /* Sauce/emulsion coats outer 3D boundary voxels and equilibrates surface temperature */
            dst->grid.T[i]        = dst->grid.T[i] * (1.0f - src_frac) + t_src * src_frac;
            dst->grid.maillard[i] = clampf(fmaxf(dst->grid.maillard[i], 0.75f * src->metrics.crust_maillard), 0.0f, 1.0f);
            dst->grid.caramel[i]  = clampf(fmaxf(dst->grid.caramel[i],  0.75f * src->metrics.crust_caramel), 0.0f, 1.0f);
        }
        if (merged_foam > 0.02f) {
            dst->grid.porosity[i] = clampf(fmaxf(dst->grid.porosity[i] * foam_keep, merged_foam), 0.0f, 0.78f);
        }
        if (src_unpast) {
            dst->grid.lethality_log10[i] = fminf(dst->grid.lethality_log10[i], src->metrics.cold_spot_lethality_log10);
        }
    }

    dst->ambient_temp_c = t_mix;
    /* Run brief 2s coupling pass so Krieger-Dougherty emulsion & thermal curdling evaluate at t_mix */
    fd_physics_step(dst, db, 2.0f);
    fd_chemistry_step(dst, db, 2.0f);
    refresh_derived(dst, db);
    push_history(dst);
    (void)coarse_mode;

    src->consumed  = true;
    src->burner_on = false;
    return true;
}

static void ensure_recipe_vessels(fd_recipe_t *r) {
    if (r->vessel_count <= 0) {
        r->vessel_count = 1;
        snprintf(r->vessels[0].id, sizeof(r->vessels[0].id), "main");
        snprintf(r->vessels[0].name, sizeof(r->vessels[0].name), "Primary Vessel");
        r->vessels[0].kind        = (r->shape == FD_SHAPE_LIQUID) ? FD_VESSEL_POT : FD_VESSEL_SKILLET;
        r->vessels[0].shape       = r->shape;
        r->vessels[0].lx_mm       = (r->lx_mm > 1.0f) ? r->lx_mm : 90.0f;
        r->vessels[0].ly_mm       = (r->ly_mm > 1.0f) ? r->ly_mm : 70.0f;
        r->vessels[0].lz_mm       = (r->lz_mm > 1.0f) ? r->lz_mm : 28.0f;
        r->vessels[0].init_temp_c = 20.0f;
    }
}

void fd_kitchen_free(fd_kitchen_t *k) {
    if (!k) return;
    for (int v = 0; v < FD_MAX_VESSELS; v++) {
        fd_sim_free(&k->vessels[v]);
    }
    memset(k, 0, sizeof(*k));
}

bool fd_kitchen_init_recipe(fd_kitchen_t *k, const fd_database_t *db,
                            const fd_recipe_t *recipe, bool coarse_mode) {
    if (!k || !db || !recipe || recipe->item_count <= 0) return false;
    fd_kitchen_free(k);
    k->recipe      = *recipe;
    ensure_recipe_vessels(&k->recipe);
    k->has_recipe  = true;
    k->coarse_mode = coarse_mode;
    k->vessel_count = k->recipe.vessel_count;

    for (int v = 0; v < k->vessel_count; v++) {
        const fd_vessel_spec_t *vs = &k->recipe.vessels[v];
        fd_sim_state_t *sv = &k->vessels[v];
        snprintf(sv->vessel_name, sizeof(sv->vessel_name), "%s", vs->name[0] ? vs->name : "Station");
        sv->vessel_kind = vs->kind;

        fd_recipe_item_t init_items[FD_MAX_RECIPE_ITEMS];
        int n_init = 0;
        for (int i = 0; i < k->recipe.item_count; i++) {
            int iv = clampf((float)k->recipe.items[i].vessel_idx, 0.0f, (float)(k->vessel_count - 1));
            if (iv == v && k->recipe.items[i].add_at_step <= 0) {
                init_items[n_init++] = k->recipe.items[i];
            }
        }
        if (n_init > 0) {
            float t0 = (vs->init_temp_c != 0.0f) ? vs->init_temp_c : 20.0f;
            fd_sim_init_dish(sv, db, init_items, n_init,
                             vs->shape, vs->lx_mm, vs->ly_mm, vs->lz_mm, t0, coarse_mode);
            sv->burner_on = (vs->kind != FD_VESSEL_BOARD && vs->kind != FD_VESSEL_BOWL && vs->kind != FD_VESSEL_PLATE);
        }
    }

    /* Configure step 0 technique on its vessel so live mode starts ready */
    if (k->recipe.step_count > 0) {
        const fd_recipe_step_t *s0 = &k->recipe.steps[0];
        int v0 = (s0->vessel_idx >= 0 && s0->vessel_idx < k->vessel_count) ? s0->vessel_idx : 0;
        k->active_vessel = v0;
        if (k->vessels[v0].initialized && s0->technique_id[0] != '\0') {
            fd_sim_set_technique(&k->vessels[v0], db, s0->technique_id, s0->temp_c, s0->lid_closed);
        }
    }
    return k->vessels[k->active_vessel].initialized || k->vessel_count > 0;
}

static void stage_items_for_step(fd_kitchen_t *k, const fd_database_t *db, int step_idx) {
    if (step_idx <= 0) return;
    for (int i = 0; i < k->recipe.item_count; i++) {
        const fd_recipe_item_t *it = &k->recipe.items[i];
        if (it->add_at_step == step_idx) {
            int v = (it->vessel_idx >= 0 && it->vessel_idx < k->vessel_count) ? it->vessel_idx : 0;
            const fd_vessel_spec_t *vs = &k->recipe.vessels[v];
            float t_add = (strstr(it->prep_note, "cold") != NULL) ? 6.0f : 20.0f;
            fd_sim_add_item(&k->vessels[v], db, it, vs->shape, vs->lx_mm, vs->ly_mm, vs->lz_mm,
                            t_add, k->coarse_mode);
        }
    }
}

bool fd_kitchen_exec_next_step(fd_kitchen_t *k, const fd_database_t *db, fd_lua_t *lua) {
    if (!k || !db || !k->has_recipe || k->current_step >= k->recipe.step_count) return false;
    int s = k->current_step;
    const fd_recipe_step_t *st = &k->recipe.steps[s];

    stage_items_for_step(k, db, s);

    int v = (st->vessel_idx >= 0 && st->vessel_idx < k->vessel_count) ? st->vessel_idx : 0;
    fd_sim_state_t *sv = &k->vessels[v];

    if (st->action == FD_ACT_COMBINE) {
        int dst_idx = (st->target_vessel_idx >= 0 && st->target_vessel_idx < k->vessel_count)
                    ? st->target_vessel_idx : 0;
        fd_sim_combine_vessels(&k->vessels[dst_idx], sv, db, lua, st->agitation <= 0.40f, k->coarse_mode);
        if (st->duration_sec > 2.0f) {
            if (st->technique_id[0] != '\0') {
                fd_sim_set_technique(&k->vessels[dst_idx], db, st->technique_id, st->temp_c, st->lid_closed);
            }
            step_chunked(&k->vessels[dst_idx], db, lua, st->duration_sec, st->flip_midway, k->coarse_mode);
        }
        k->active_vessel = dst_idx;
    } else if (st->action == FD_ACT_FOLD_STIR && st->target_vessel_idx >= 0 &&
               st->target_vessel_idx < k->vessel_count && st->target_vessel_idx != v) {
        int dst_idx = st->target_vessel_idx;
        fd_sim_combine_vessels(&k->vessels[dst_idx], sv, db, lua, st->agitation <= 0.45f, k->coarse_mode);
        fd_sim_apply_action(&k->vessels[dst_idx], db, lua, FD_ACT_FOLD_STIR, st->agitation,
                            0.0f, 0.0f, 0.0f, FD_SHAPE_SLAB, false, st->temp_c, st->duration_sec, k->coarse_mode);
        k->active_vessel = dst_idx;
    } else if (st->action == FD_ACT_COOK) {
        if (sv->initialized) {
            fd_sim_set_technique(sv, db, st->technique_id, st->temp_c, st->lid_closed);
            step_chunked(sv, db, lua, st->duration_sec, st->flip_midway, k->coarse_mode);
        }
        k->active_vessel = v;
    } else {
        if (sv->initialized) {
            fd_sim_apply_action(sv, db, lua, st->action, st->agitation,
                                st->cut_lx_mm, st->cut_ly_mm, st->cut_lz_mm,
                                st->cut_shape, st->has_cut_shape,
                                st->temp_c, st->duration_sec, k->coarse_mode);
        }
        if (st->action == FD_ACT_DEGLAZE && st->target_vessel_idx >= 0 &&
            st->target_vessel_idx < k->vessel_count && st->target_vessel_idx != v) {
            fd_sim_combine_vessels(&k->vessels[st->target_vessel_idx], sv, db, lua, false, k->coarse_mode);
            k->active_vessel = st->target_vessel_idx;
        } else {
            k->active_vessel = v;
        }
    }

    if (!st->parallel) k->wall_time_sec += (double)fmaxf(0.0f, st->duration_sec);
    k->current_step++;

    /* Post-recipe cold refrigerator RS3 retrogradation hold if configured */
    if (k->current_step == k->recipe.step_count && k->recipe.chill_after) {
        fd_sim_state_t *fin = &k->vessels[k->active_vessel];
        if (fin->initialized) {
            fin->ambient_temp_c = 4.0f;
            for (int c = 0; c < 24; c++) fd_sim_step(fin, db, lua, 600.0f);
        }
    }
    return true;
}

void fd_kitchen_step_live(fd_kitchen_t *k, const fd_database_t *db, fd_lua_t *lua, float dt_sec) {
    if (!k || !db || dt_sec <= 0.0f) return;
    float max_siz = 0.0f, max_boil = 0.0f;
    for (int v = 0; v < k->vessel_count; v++) {
        fd_sim_state_t *sv = &k->vessels[v];
        if (!sv->initialized || sv->consumed) continue;
        if (sv->burner_on || fabsf(sv->ambient_temp_c - sv->metrics.core_temp_c) > 1.5f) {
            fd_sim_step(sv, db, lua, dt_sec);
            if (sv->sizzle_flux > max_siz) max_siz = sv->sizzle_flux;
            if (sv->boil_flux > max_boil)  max_boil = sv->boil_flux;
        }
    }
    k->wall_time_sec += (double)dt_sec;
    k->sizzle_flux = max_siz;
    k->boil_flux   = max_boil;
}

bool fd_kitchen_run_recipe(fd_kitchen_t *k, const fd_database_t *db, fd_lua_t *lua,
                           const fd_recipe_t *recipe, bool coarse_mode) {
    if (!k || !db || !recipe || recipe->item_count <= 0 || recipe->step_count <= 0) return false;
    if (!fd_kitchen_init_recipe(k, db, recipe, coarse_mode)) return false;
    while (k->current_step < k->recipe.step_count) {
        fd_kitchen_exec_next_step(k, db, lua);
    }
    /* Select primary surviving non-consumed vessel with largest cooked weight */
    int best_v = k->active_vessel;
    float best_w = -1.0f;
    for (int v = 0; v < k->vessel_count; v++) {
        if (k->vessels[v].initialized && !k->vessels[v].consumed &&
            k->vessels[v].metrics.cooked_weight_g > best_w) {
            best_w = k->vessels[v].metrics.cooked_weight_g;
            best_v = v;
        }
    }
    k->active_vessel = best_v;
    return k->vessels[best_v].initialized;
}

bool fd_sim_run_recipe(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua,
                       const fd_recipe_t *recipe, bool coarse_mode) {
    if (!sim || !db || !recipe || recipe->item_count <= 0 || recipe->step_count <= 0) return false;
    fd_kitchen_t *k = (fd_kitchen_t *)calloc(1, sizeof(fd_kitchen_t));
    if (!k) return false;
    bool ok = fd_kitchen_run_recipe(k, db, lua, recipe, coarse_mode);
    if (ok) {
        fd_sim_free(sim);
        int av = k->active_vessel;
        *sim = k->vessels[av];
        memset(&k->vessels[av].grid, 0, sizeof(k->vessels[av].grid));
    }
    fd_kitchen_free(k);
    free(k);
    return ok;
}

static const char *action_verb(fd_step_action_t a) {
    switch (a) {
        case FD_ACT_CUT_PREP:   return "Knife Prep / Cut";
        case FD_ACT_POUND:      return "Pound & Tenderize";
        case FD_ACT_SALT_BRINE: return "Salt & Dry-Brine";
        case FD_ACT_WHISK:      return "Whisk / Whip";
        case FD_ACT_KNEAD:      return "Knead & Stretch";
        case FD_ACT_FOLD_STIR:  return "Fold / Stir";
        case FD_ACT_DEGLAZE:    return "Deglaze Pan Fond";
        case FD_ACT_COMBINE:    return "Transfer & Combine";
        case FD_ACT_REST:       return "Rest Off-Heat";
        case FD_ACT_COOK:
        default:                return "Cook";
    }
}

void fd_recipe_describe_step(const fd_recipe_t *r, const fd_database_t *db, int step_idx,
                             char *out_instr, size_t instr_cap,
                             char *out_why, size_t why_cap) {
    if (out_instr && instr_cap > 0) out_instr[0] = '\0';
    if (out_why && why_cap > 0)     out_why[0]   = '\0';
    if (!r || step_idx < 0 || step_idx >= r->step_count) return;

    const fd_recipe_step_t *st = &r->steps[step_idx];
    int v = (st->vessel_idx >= 0 && st->vessel_idx < r->vessel_count) ? st->vessel_idx : 0;
    const char *vname = (r->vessel_count > 0 && r->vessels[v].name[0]) ? r->vessels[v].name : "Primary Vessel";

    if (out_instr && instr_cap > 0) {
        if (st->instruction[0] != '\0') {
            snprintf(out_instr, instr_cap, "[%s%s] %s",
                     vname, st->parallel ? " || Parallel" : "", st->instruction);
        } else {
            /* Synthesize exact chef-executable step with staged gram weights, mm cuts, vessel, temp & time */
            char staged[120] = "";
            size_t slen = 0;
            for (int i = 0; i < r->item_count; i++) {
                int iv = (r->items[i].vessel_idx >= 0 && r->items[i].vessel_idx < r->vessel_count)
                       ? r->items[i].vessel_idx : 0;
                bool match = (step_idx == 0 && r->items[i].add_at_step == 0 && iv == v) ||
                             (step_idx > 0  && r->items[i].add_at_step == step_idx && iv == v);
                if (match && slen + 28 < sizeof(staged)) {
                    int ii = db ? fd_db_find_ingredient(db, r->items[i].ingredient_id) : -1;
                    const char *inm = (ii >= 0) ? db->ingredients[ii].name : r->items[i].ingredient_id;
                    slen += (size_t)snprintf(staged + slen, sizeof(staged) - slen, "%s%.0fg %s",
                                             slen > 0 ? ", " : "", r->items[i].mass_g, inm);
                }
            }
            int ti = db ? fd_db_find_technique(db, st->technique_id) : -1;
            const char *tname = (ti >= 0) ? db->techniques[ti].name : st->technique_id;

            if (st->action == FD_ACT_COMBINE && st->target_vessel_idx >= 0 && st->target_vessel_idx < r->vessel_count) {
                snprintf(out_instr, instr_cap, "[%s -> %s] Transfer & combine contents into %s (%.0fs%s).",
                         vname, r->vessels[st->target_vessel_idx].name, r->vessels[st->target_vessel_idx].name,
                         st->duration_sec, st->agitation <= 0.4f ? ", gentle fold" : ", toss to emulsify");
            } else if (st->action == FD_ACT_CUT_PREP) {
                snprintf(out_instr, instr_cap, "[%s] %s%s%s%s to %.0fx%.0fx%.0fmm uniform pieces.",
                         vname, action_verb(st->action), staged[0] ? " (" : "", staged, staged[0] ? ")" : "",
                         st->cut_lx_mm > 0 ? st->cut_lx_mm : r->lx_mm,
                         st->cut_ly_mm > 0 ? st->cut_ly_mm : r->ly_mm,
                         st->cut_lz_mm > 0 ? st->cut_lz_mm : r->lz_mm);
            } else if (st->action != FD_ACT_COOK) {
                snprintf(out_instr, instr_cap, "[%s%s] %s%s%s at %.0f°C for %.0fs.",
                         vname, st->parallel ? " ||" : "", action_verb(st->action),
                         staged[0] ? ": " : "", staged, st->temp_c, st->duration_sec);
            } else {
                snprintf(out_instr, instr_cap, "[%s%s] %s%s%s at %.0f°C for %.0fs (%0.1f min)%s%s.",
                         vname, st->parallel ? " ||" : "", tname,
                         staged[0] ? " with " : "", staged,
                         st->temp_c, st->duration_sec, st->duration_sec / 60.0f,
                         st->flip_midway ? ", flip/toss halfway" : "",
                         st->lid_closed ? ", lid closed" : "");
            }
        }
    }

    if (out_why && why_cap > 0) {
        if (st->cue_why[0] != '\0') {
            snprintf(out_why, why_cap, "%s", st->cue_why);
        } else if (st->action == FD_ACT_CUT_PREP) {
            snprintf(out_why, why_cap, "Sets 3D conduction path length (L^2 scaling) and ruptures cells to release aroma & Maillard amines.");
        } else if (st->action == FD_ACT_WHISK) {
            snprintf(out_why, why_cap, "High shear shrinks oil droplets (<5 um) for Krieger-Dougherty stability and whips air porosity.");
        } else if (st->action == FD_ACT_KNEAD) {
            snprintf(out_why, why_cap, "Aligns hydrated gliadin/glutenin into a viscoelastic gluten film that traps fermentation CO2.");
        } else if (st->action == FD_ACT_FOLD_STIR) {
            snprintf(out_why, why_cap, "Low-shear folding homogenizes batter while preserving >=90%% of whipped air foam porosity.");
        } else if (st->action == FD_ACT_DEGLAZE) {
            snprintf(out_why, why_cap, "Dissolves browned pan melanoidins and glutamate fond into the sauce while quenching pan heat.");
        } else if (st->action == FD_ACT_REST) {
            snprintf(out_why, why_cap, "3D ADI carryover equilibrates core temperature while cooling myofibrils reabsorb juices.");
        } else if (st->action == FD_ACT_COMBINE) {
            snprintf(out_why, why_cap, "Conserves enthalpy T_mix = (C1*T1 + C2*T2)/(C1+C2) to marry components without thermal curdling.");
        } else if (st->temp_c >= 165.0f) {
            snprintf(out_why, why_cap, "Desiccates surface past 100°C boiling plateau to drive golden Maillard crust (>140°C) below char.");
        } else if (st->temp_c >= 80.0f) {
            snprintf(out_why, why_cap, "Hydrates starch (>60°C) or hydrolyzes tough collagen into velvety gelatin (>68°C) with 6.5D safety.");
        } else {
            snprintf(out_why, why_cap, "Gentle isothermal control hits pasteurization or emulsion target below actin/yolk curdling.");
        }
    }
}
