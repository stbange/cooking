#include "fd_solver.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state ? *state : 0xFA7D00C5u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static float rand01(uint32_t *state) {
    return (float)(xorshift32(state) & 0xFFFFFFu) / 16777216.0f;
}

void fd_constraints_init_default(fd_constraints_t *c) {
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->max_time_sec      = 3600.0f;
    c->max_kcal_per_100g = 0.0f;
    c->max_gi            = 0.0f;
    c->min_protein_g     = 0.0f;
    c->target_crispness  = -1.0f;
    c->target_tenderness = -1.0f;
    c->target_umami      = -1.0f;
    c->target_sweet      = -1.0f;
    c->target_sour       = -1.0f;
    c->rng_seed          = 0xFA7D00C5u;
}

static bool is_excluded(const fd_constraints_t *c, const fd_ingredient_t *ing) {
    if ((c->required_diet_flags & ing->diet_flags) != c->required_diet_flags) return true;
    for (int i = 0; i < c->exclude_count; i++) {
        if (strcmp(c->exclude_ids[i], ing->id) == 0) return true;
    }
    return false;
}

float fd_ingredient_similarity(const fd_ingredient_t *a, const fd_ingredient_t *b) {
    if (!a || !b) return 0.0f;
    float va[9] = {
        a->water, a->protein * 2.0f, a->fat * 2.0f, a->starch * 2.0f, a->sugar * 2.0f,
        a->emulsifier_power * 1.5f, (a->ph - 4.5f) * 0.25f,
        a->glutamate_mg_100g * 0.002f, a->acidity_molar * 2.0f
    };
    float vb[9] = {
        b->water, b->protein * 2.0f, b->fat * 2.0f, b->starch * 2.0f, b->sugar * 2.0f,
        b->emulsifier_power * 1.5f, (b->ph - 4.5f) * 0.25f,
        b->glutamate_mg_100g * 0.002f, b->acidity_molar * 2.0f
    };
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (int i = 0; i < 9; i++) {
        dot += va[i] * vb[i];
        na  += va[i] * va[i];
        nb  += vb[i] * vb[i];
    }
    float cos_sim = (na > 1e-6f && nb > 1e-6f) ? (dot / sqrtf(na * nb)) : 0.0f;
    float role_overlap = (a->role_flags & b->role_flags) ? 0.18f : 0.0f;
    return clampf(cos_sim * 0.85f + role_overlap, 0.0f, 1.0f);
}

static float recipe_total_time(const fd_recipe_t *r) {
    float t = 0.0f;
    for (int i = 0; i < r->step_count; i++) t += r->steps[i].duration_sec;
    return t;
}

float fd_evaluate_fitness(const fd_metrics_t *m, const fd_recipe_t *r, const fd_constraints_t *c) {
    if (!m || !r || !c) return -999.0f;
    float fit = m->overall_score;

    /* Hard physical, microbial safety, charring & constraint gates */
    if (!m->is_pasteurized_safe) fit -= 75.0f;
    if (m->max_char > 0.14f)     fit -= 60.0f * m->max_char;
    if (m->oil_smoked)           fit -= 22.0f;
    if (c->require_emulsion && m->emulsion_stability < 0.65f) {
        fit -= 55.0f * (0.70f - m->emulsion_stability);
    }

    float tot_t = recipe_total_time(r);
    if (c->max_time_sec > 0.0f && tot_t > c->max_time_sec) {
        fit -= 0.05f * (tot_t - c->max_time_sec);
    }
    if (c->max_kcal_per_100g > 0.0f && m->kcal_per_100g > c->max_kcal_per_100g) {
        fit -= 0.35f * (m->kcal_per_100g - c->max_kcal_per_100g);
    }
    if (c->max_gi > 0.0f && m->estimated_gi > c->max_gi) {
        fit -= 1.8f * (m->estimated_gi - c->max_gi);
    }
    if (c->min_protein_g > 0.0f && m->protein_g < c->min_protein_g) {
        fit -= 2.5f * (c->min_protein_g - m->protein_g);
    }

    /* Target sensory alignment bonuses/penalties */
    if (c->target_crispness >= 0.0f)  fit += 6.0f - 2.2f * fabsf(m->crispness  - c->target_crispness);
    if (c->target_tenderness >= 0.0f) fit += 6.0f - 2.2f * fabsf(m->tenderness - c->target_tenderness);
    if (c->target_umami >= 0.0f)      fit += 6.0f - 2.0f * fabsf(m->umami      - c->target_umami);
    if (c->target_sweet >= 0.0f)      fit += 6.0f - 2.0f * fabsf(m->sweet      - c->target_sweet);
    if (c->target_sour >= 0.0f)       fit += 6.0f - 2.0f * fabsf(m->sour       - c->target_sour);

    /* Reward inclusion of requested pantry items */
    for (int k = 0; k < c->include_count; k++) {
        bool present = false;
        for (int i = 0; i < r->item_count; i++) {
            if (strcmp(r->items[i].ingredient_id, c->include_ids[k]) == 0) {
                present = true;
                break;
            }
        }
        fit += present ? 10.0f : -35.0f;
    }
    return fit;
}

static int find_best_substitute(const fd_database_t *db, int orig_idx, const fd_constraints_t *c) {
    const fd_ingredient_t *orig = &db->ingredients[orig_idx];
    int best = -1;
    float best_sim = 0.42f; /* Minimum chemical similarity threshold */
    for (int i = 0; i < db->ingredient_count; i++) {
        if (i == orig_idx || is_excluded(c, &db->ingredients[i])) continue;
        float s = fd_ingredient_similarity(orig, &db->ingredients[i]);
        if (s > best_sim) {
            best_sim = s;
            best = i;
        }
    }
    return best;
}

int fd_solver_propose_classics(const fd_database_t *db, fd_lua_t *lua,
                               const fd_constraints_t *c,
                               fd_proposal_t *out_proposals, int max_out) {
    if (!db || !c || !out_proposals || max_out <= 0) return 0;
    int count = 0;

    for (int r_i = 0; r_i < db->recipe_count; r_i++) {
        fd_recipe_t cand = db->recipes[r_i];
        if (c->max_time_sec > 0.0f && recipe_total_time(&cand) > c->max_time_sec * 1.35f) continue;

        int subs = 0;
        bool viable = true;
        char sub_note[128] = "";

        for (int i = 0; i < cand.item_count; i++) {
            int idx = fd_db_find_ingredient(db, cand.items[i].ingredient_id);
            if (idx < 0) { viable = false; break; }
            if (is_excluded(c, &db->ingredients[idx])) {
                int rep = find_best_substitute(db, idx, c);
                if (rep < 0 || subs >= 3) { viable = false; break; }
                if (subs == 0) {
                    snprintf(sub_note, sizeof(sub_note), "Substituted %.44s -> %.44s",
                             db->ingredients[idx].name, db->ingredients[rep].name);
                }
                snprintf(cand.items[i].ingredient_id, sizeof(cand.items[i].ingredient_id),
                         "%s", db->ingredients[rep].id);
                subs++;
            }
        }
        if (!viable) continue;

        /* Verify candidate on coarse 3D grid first */
        fd_sim_state_t sim;
        memset(&sim, 0, sizeof(sim));
        if (!fd_sim_run_recipe(&sim, db, lua, &cand, true)) {
            fd_sim_free(&sim);
            continue;
        }
        float fit = fd_evaluate_fitness(&sim.metrics, &cand, c) - 2.5f * (float)subs;

        /* Insert into sorted top proposals */
        int ins = -1;
        if (count < max_out) {
            ins = count++;
        } else if (fit > out_proposals[count - 1].fitness) {
            ins = count - 1;
        }
        if (ins >= 0) {
            out_proposals[ins].recipe        = cand;
            out_proposals[ins].metrics       = sim.metrics;
            out_proposals[ins].fitness       = fit;
            out_proposals[ins].substitutions = subs;
            snprintf(out_proposals[ins].rationale, sizeof(out_proposals[ins].rationale),
                     "%s", (subs > 0) ? sub_note : "Matches pantry & constraints natively");
            while (ins > 0 && out_proposals[ins].fitness > out_proposals[ins - 1].fitness) {
                fd_proposal_t tmp = out_proposals[ins - 1];
                out_proposals[ins - 1] = out_proposals[ins];
                out_proposals[ins] = tmp;
                ins--;
            }
        }
        fd_sim_free(&sim);
    }

    /* Upgrade #1 proposal to full 3D resolution verification */
    if (count > 0) {
        fd_sim_state_t full_sim;
        memset(&full_sim, 0, sizeof(full_sim));
        if (fd_sim_run_recipe(&full_sim, db, lua, &out_proposals[0].recipe, false)) {
            out_proposals[0].metrics = full_sim.metrics;
            out_proposals[0].fitness = fd_evaluate_fitness(&full_sim.metrics, &out_proposals[0].recipe, c)
                                     - 2.5f * (float)out_proposals[0].substitutions;
        }
        fd_sim_free(&full_sim);
    }
    return count;
}

static int pick_by_role(const fd_database_t *db, const fd_constraints_t *c, uint32_t role_mask, uint32_t *rng) {
    int matches[FD_MAX_INGREDIENTS];
    int n = 0;
    for (int i = 0; i < db->ingredient_count; i++) {
        if (is_excluded(c, &db->ingredients[i])) continue;
        if (db->ingredients[i].role_flags & role_mask) {
            matches[n++] = i;
        }
    }
    if (n == 0) return -1;
    return matches[xorshift32(rng) % (uint32_t)n];
}

static void add_unique_item(fd_recipe_t *r, const char *id, float mass_g) {
    if (!id || id[0] == '\0' || r->item_count >= FD_MAX_RECIPE_ITEMS) return;
    for (int i = 0; i < r->item_count; i++) {
        if (strcmp(r->items[i].ingredient_id, id) == 0) return;
    }
    snprintf(r->items[r->item_count].ingredient_id, sizeof(r->items[0].ingredient_id), "%s", id);
    r->items[r->item_count].mass_g = mass_g;
    r->item_count++;
}

static void build_random_blueprint(fd_recipe_t *r, const fd_database_t *db,
                                   const fd_constraints_t *c, uint32_t *rng, int idx_num) {
    memset(r, 0, sizeof(*r));
    snprintf(r->id, sizeof(r->id), "invented_%d", idx_num + 1);
    snprintf(r->category, sizeof(r->category), "invented");

    /* 1. Include mandatory user pantry ingredients first */
    int main_idx = -1;
    for (int i = 0; i < c->include_count; i++) {
        int id_idx = fd_db_find_ingredient(db, c->include_ids[i]);
        if (id_idx >= 0 && !is_excluded(c, &db->ingredients[id_idx])) {
            uint32_t rf = db->ingredients[id_idx].role_flags;
            float def_m = (rf & (FD_ROLE_PROTEIN_MAIN | FD_ROLE_VEG_MAIN | FD_ROLE_STARCH_BASE)) ? 180.0f : 22.0f;
            if (rf & FD_ROLE_SALT) def_m = 2.4f;
            add_unique_item(r, db->ingredients[id_idx].id, def_m);
            if (main_idx < 0 && (rf & (FD_ROLE_PROTEIN_MAIN | FD_ROLE_VEG_MAIN | FD_ROLE_STARCH_BASE))) {
                main_idx = id_idx;
            }
        }
    }

    /* 2. Ensure a primary structural food or emulsion base exists */
    if (main_idx < 0) {
        uint32_t main_roles = c->require_emulsion
            ? (FD_ROLE_LIQUID_BASE | FD_ROLE_EMULSIFIER)
            : (FD_ROLE_PROTEIN_MAIN | FD_ROLE_VEG_MAIN | FD_ROLE_STARCH_BASE);
        main_idx = pick_by_role(db, c, main_roles, rng);
        if (main_idx < 0) main_idx = pick_by_role(db, c, 0xFFFFFFFFu, rng);
        if (main_idx >= 0) add_unique_item(r, db->ingredients[main_idx].id, c->require_emulsion ? 60.0f : 190.0f);
    }

    /* 3. Add fat/cooking medium, salt, and flavor enhancers (umami/acid/emulsifier/aromatic) */
    int fat_idx = pick_by_role(db, c, FD_ROLE_FAT_MEDIUM, rng);
    if (fat_idx >= 0) add_unique_item(r, db->ingredients[fat_idx].id, c->require_emulsion ? 95.0f : 18.0f);

    if (c->require_emulsion) {
        int em_idx = pick_by_role(db, c, FD_ROLE_EMULSIFIER, rng);
        if (em_idx >= 0) add_unique_item(r, db->ingredients[em_idx].id, 28.0f);
        int liq_idx = pick_by_role(db, c, FD_ROLE_LIQUID_BASE | FD_ROLE_ACID, rng);
        if (liq_idx >= 0) add_unique_item(r, db->ingredients[liq_idx].id, 35.0f);
    }

    int salt_idx = pick_by_role(db, c, FD_ROLE_SALT, rng);
    if (salt_idx >= 0) add_unique_item(r, db->ingredients[salt_idx].id, 2.2f);

    uint32_t accent_role = FD_ROLE_AROMATIC | FD_ROLE_UMAMI;
    if (c->target_sour > 3.0f || c->max_gi > 0.0f) accent_role |= FD_ROLE_ACID;
    if (c->target_sweet > 4.0f) accent_role |= FD_ROLE_SWEETENER;
    int acc_idx = pick_by_role(db, c, accent_role, rng);
    if (acc_idx >= 0) add_unique_item(r, db->ingredients[acc_idx].id, 16.0f);

    /* 4. Geometry & 1-2 stage cooking technique schedule */
    if (c->require_emulsion) {
        r->shape = FD_SHAPE_LIQUID;
        r->lx_mm = 80.0f; r->ly_mm = 80.0f; r->lz_mm = 40.0f;
    } else if (main_idx >= 0) {
        const fd_ingredient_t *mi = &db->ingredients[main_idx];
        r->shape = mi->default_shape;
        r->lx_mm = mi->default_lx_mm;
        r->ly_mm = mi->default_ly_mm;
        r->lz_mm = mi->default_lz_mm;
    } else {
        r->shape = FD_SHAPE_SLAB;
        r->lx_mm = 90.0f; r->ly_mm = 70.0f; r->lz_mm = 24.0f;
    }

    float max_t = (c->max_time_sec > 60.0f) ? c->max_time_sec : 1500.0f;
    if (c->require_emulsion && fd_db_find_technique(db, "whisk_emulsify") >= 0) {
        r->step_count = 1;
        snprintf(r->steps[0].technique_id, sizeof(r->steps[0].technique_id), "whisk_emulsify");
        r->steps[0].temp_c       = 58.0f + 10.0f * rand01(rng);
        r->steps[0].duration_sec = 180.0f + 120.0f * rand01(rng);
    } else if (db->technique_count > 0) {
        bool two_step = (max_t >= 480.0f) && (rand01(rng) > 0.35f);
        r->step_count = two_step ? 2 : 1;
        for (int s = 0; s < r->step_count; s++) {
            int ti = (int)(xorshift32(rng) % (uint32_t)db->technique_count);
            /* Prefer high-heat sear on final step if crispness desired */
            if (s == r->step_count - 1 && c->target_crispness > 5.0f && fd_db_find_technique(db, "pan_sear") >= 0) {
                ti = fd_db_find_technique(db, "pan_sear");
            } else if (s == 0 && two_step && fd_db_find_technique(db, "sous_vide") >= 0 && max_t >= 900.0f) {
                ti = fd_db_find_technique(db, "sous_vide");
            }
            const fd_technique_t *tc = &db->techniques[ti];
            snprintf(r->steps[s].technique_id, sizeof(r->steps[s].technique_id), "%s", tc->id);
            r->steps[s].temp_c       = clampf(tc->default_temp_c + (rand01(rng) - 0.5f) * 28.0f, tc->min_temp_c, tc->max_temp_c);
            r->steps[s].duration_sec = clampf((0.25f + 0.45f * rand01(rng)) * (max_t / (float)r->step_count), 60.0f, max_t * 0.7f);
            r->steps[s].flip_midway  = (r->shape != FD_SHAPE_LIQUID);
            r->steps[s].lid_closed   = (s == 0 && two_step);
        }
    }

    const char *mname = (main_idx >= 0) ? db->ingredients[main_idx].name : "Culinary Creation";
    const char *aname = (acc_idx >= 0)  ? db->ingredients[acc_idx].name  : "Artisanal";
    snprintf(r->name, sizeof(r->name), "Invented %.26s & %.26s", mname, aname);
    snprintf(r->notes, sizeof(r->notes), "Synthesized from 3D first-principles evolutionary search.");
}

static void mutate_recipe(fd_recipe_t *r, const fd_database_t *db, const fd_constraints_t *c, uint32_t *rng) {
    /* Perturb masses */
    for (int i = 0; i < r->item_count; i++) {
        if (rand01(rng) < 0.65f) {
            float scale = 0.78f + 0.44f * rand01(rng);
            r->items[i].mass_g = clampf(r->items[i].mass_g * scale, 0.8f, 450.0f);
        }
    }
    /* Perturb 3D cut thickness lz_mm */
    if (r->shape != FD_SHAPE_LIQUID && rand01(rng) < 0.45f) {
        r->lz_mm = clampf(r->lz_mm * (0.82f + 0.36f * rand01(rng)), 8.0f, 65.0f);
    }
    /* Perturb cooking step temperatures & durations */
    float max_t = (c->max_time_sec > 60.0f) ? c->max_time_sec : 1800.0f;
    for (int s = 0; s < r->step_count; s++) {
        int ti = fd_db_find_technique(db, r->steps[s].technique_id);
        float lo = (ti >= 0) ? db->techniques[ti].min_temp_c : 40.0f;
        float hi = (ti >= 0) ? db->techniques[ti].max_temp_c : 240.0f;
        if (rand01(rng) < 0.75f) {
            r->steps[s].temp_c = clampf(r->steps[s].temp_c + (rand01(rng) - 0.5f) * 24.0f, lo, hi);
        }
        if (rand01(rng) < 0.75f) {
            r->steps[s].duration_sec = clampf(r->steps[s].duration_sec * (0.72f + 0.56f * rand01(rng)),
                                              35.0f, max_t / (float)r->step_count);
        }
    }
}

bool fd_inventor_start(fd_inventor_t *inv, const fd_database_t *db, const fd_constraints_t *c) {
    if (!inv || !db || !c || db->ingredient_count == 0 || db->technique_count == 0) return false;
    memset(inv, 0, sizeof(*inv));
    inv->constraints  = *c;
    inv->rng          = c->rng_seed ? c->rng_seed : 0xFA7D00C5u;
    inv->active       = true;
    inv->done         = false;
    inv->evals_target = 54; /* 6 initial + 48 evolutionary offspring on 8x8x8 3D grid + full 3D verify */
    inv->best.fitness = -1e9f;

    for (int i = 0; i < FD_INV_POP_SIZE; i++) {
        build_random_blueprint(&inv->pop[i], db, c, &inv->rng, i);
        inv->pop_fitness[i] = -1e9f;
    }
    inv->pop_count = FD_INV_POP_SIZE;
    return true;
}

bool fd_inventor_step(fd_inventor_t *inv, const fd_database_t *db, fd_lua_t *lua, int budget_evals) {
    if (!inv || !inv->active || !db) return true;
    if (inv->done) return true;
    if (budget_evals <= 0) budget_evals = 8;

    fd_sim_state_t sim;
    memset(&sim, 0, sizeof(sim));

    int ran = 0;
    while (ran < budget_evals && inv->evals_done < inv->evals_target) {
        if (inv->evals_done < inv->pop_count) {
            int p = inv->evals_done;
            if (fd_sim_run_recipe(&sim, db, lua, &inv->pop[p], true)) {
                inv->pop_metrics[p] = sim.metrics;
                inv->pop_fitness[p] = fd_evaluate_fitness(&sim.metrics, &inv->pop[p], &inv->constraints);
                if (inv->pop_fitness[p] > inv->best.fitness) {
                    inv->best.recipe  = inv->pop[p];
                    inv->best.metrics = sim.metrics;
                    inv->best.fitness = inv->pop_fitness[p];
                }
            }
            fd_sim_free(&sim);
        } else {
            int parent = inv->evals_done % inv->pop_count;
            fd_recipe_t child = inv->pop[parent];
            mutate_recipe(&child, db, &inv->constraints, &inv->rng);
            if (fd_sim_run_recipe(&sim, db, lua, &child, true)) {
                float fit = fd_evaluate_fitness(&sim.metrics, &child, &inv->constraints);
                if (fit >= inv->pop_fitness[parent]) {
                    inv->pop[parent]         = child;
                    inv->pop_metrics[parent] = sim.metrics;
                    inv->pop_fitness[parent] = fit;
                }
                if (fit > inv->best.fitness) {
                    inv->best.recipe  = child;
                    inv->best.metrics = sim.metrics;
                    inv->best.fitness = fit;
                }
            }
            fd_sim_free(&sim);
        }
        inv->evals_done++;
        ran++;
    }

    if (inv->evals_done >= inv->evals_target) {
        /* Promote top coarse winners to full-resolution 3D grid verification! */
        int best_p = 0;
        for (int i = 1; i < inv->pop_count; i++) {
            if (inv->pop_fitness[i] > inv->pop_fitness[best_p]) best_p = i;
        }
        if (fd_sim_run_recipe(&sim, db, lua, &inv->pop[best_p], false)) {
            inv->best.recipe  = inv->pop[best_p];
            inv->best.metrics = sim.metrics;
            inv->best.fitness = fd_evaluate_fitness(&sim.metrics, &inv->pop[best_p], &inv->constraints);
            snprintf(inv->best.rationale, sizeof(inv->best.rationale),
                     "Verified on full %dx%dx%d 3D PDE grid (Cold-spot %.1f log10, eGI %.0f)",
                     sim.grid.nx, sim.grid.ny, sim.grid.nz,
                     sim.metrics.cold_spot_lethality_log10, sim.metrics.estimated_gi);
        }
        fd_sim_free(&sim);
        inv->done   = true;
        inv->active = false;
        return true;
    }
    return false;
}

float fd_inventor_progress(const fd_inventor_t *inv) {
    if (!inv || inv->evals_target <= 0) return 0.0f;
    if (inv->done) return 1.0f;
    return clampf((float)inv->evals_done / (float)inv->evals_target, 0.0f, 0.99f);
}
