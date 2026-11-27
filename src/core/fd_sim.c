#include "fd_sim.h"
#include "fd_biology.h"
#include "fd_chemistry.h"
#include "fd_geometry.h"
#include "fd_nutrition.h"
#include "fd_physics.h"

#include <math.h>
#include <string.h>

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
    fd_sim_free(sim);

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

    int nx = 1, ny = 1, nz = 1;
    fd_geometry_choose_dims(shape, lx_mm, ly_mm, lz_mm, coarse_mode, &nx, &ny, &nz);
    if (!fd_grid_alloc(&sim->grid, nx, ny, nz, lx_mm, ly_mm, lz_mm)) return false;

    fd_grid_rasterize(&sim->grid, shape, init_temp_c,
                      fminf(0.94f, sim->total_water_g * w_inv),
                      fminf(0.85f, sim->total_fat_g * w_inv),
                      fminf(0.12f, sim->salt_g * w_inv),
                      weighted_aniso);

    sim->ambient_temp_c = 180.0f;
    sim->initialized = true;

    float base_ph = (sim->primary_ing_idx >= 0) ? db->ingredients[sim->primary_ing_idx].ph : 6.1f;
    float aq_mass_g = fmaxf(8.0f, sim->total_water_g + 0.35f * sim->dry_protein_g);
    sim->metrics.ph = fd_compute_ph(sim->acid_mol, sim->base_mol, aq_mass_g, base_ph);
    sim->metrics.emulsion_stability = 1.0f;
    fd_biology_step(sim, db, 0.0f);
    fd_nutrition_update(sim);
    push_history(sim);
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
    return true;
}

void fd_sim_step(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua, float dt_sec) {
    if (!sim || !sim->initialized || !db || dt_sec <= 0.0f) return;

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

bool fd_sim_run_recipe(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua,
                       const fd_recipe_t *recipe, bool coarse_mode) {
    if (!sim || !db || !recipe || recipe->item_count <= 0 || recipe->step_count <= 0) return false;
    if (!fd_sim_init_dish(sim, db, recipe->items, recipe->item_count,
                          recipe->shape, recipe->lx_mm, recipe->ly_mm, recipe->lz_mm,
                          20.0f, coarse_mode)) {
        return false;
    }

    for (int s = 0; s < recipe->step_count; s++) {
        const fd_recipe_step_t *st = &recipe->steps[s];
        fd_sim_set_technique(sim, db, st->technique_id, st->temp_c, st->lid_closed);
        float dur = fmaxf(1.0f, st->duration_sec);
        /* Choose adaptive step chunk (coarse search runs larger steps safely via unconditionally stable ADI) */
        float chunk = coarse_mode ? fminf(30.0f, fmaxf(4.0f, dur / 12.0f))
                                  : fminf(15.0f, fmaxf(2.0f, dur / 36.0f));
        float elapsed = 0.0f;
        bool flipped = false;
        while (elapsed < dur - 1e-3f) {
            float dt = fminf(chunk, dur - elapsed);
            if (st->flip_midway && !flipped && elapsed >= dur * 0.5f) {
                if (recipe->shape == FD_SHAPE_CUBE) fd_grid_toss_cubes(&sim->grid);
                else fd_grid_flip_z(&sim->grid);
                flipped = true;
            }
            fd_sim_step(sim, db, lua, dt);
            elapsed += dt;
        }
    }

    if (recipe->chill_after) {
        /* Cold refrigerator retrogradation hold (4°C for 4 hours) */
        sim->ambient_temp_c = 4.0f;
        for (int k = 0; k < 24; k++) {
            fd_sim_step(sim, db, lua, 600.0f);
        }
    }
    return true;
}
