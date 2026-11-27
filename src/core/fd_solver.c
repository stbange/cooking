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
    for (int i = 0; i < r->step_count; i++) {
        if (!r->steps[i].parallel) t += r->steps[i].duration_sec;
    }
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

static void add_unique_item_v(fd_recipe_t *r, const char *id, float mass_g, int vessel_idx, const char *prep) {
    if (!id || id[0] == '\0' || r->item_count >= FD_MAX_RECIPE_ITEMS) return;
    for (int i = 0; i < r->item_count; i++) {
        if (strcmp(r->items[i].ingredient_id, id) == 0) return;
    }
    fd_recipe_item_t *it = &r->items[r->item_count++];
    snprintf(it->ingredient_id, sizeof(it->ingredient_id), "%s", id);
    it->mass_g     = mass_g;
    it->vessel_idx = vessel_idx;
    it->add_at_step = 0;
    snprintf(it->prep_note, sizeof(it->prep_note), "%s", prep ? prep : "");
}

static int pick_thermal_technique(const fd_database_t *db, uint32_t *rng) {
    int matches[FD_MAX_TECHNIQUES];
    int n = 0;
    for (int i = 0; i < db->technique_count; i++) {
        if (db->techniques[i].default_temp_c >= 54.0f) matches[n++] = i;
    }
    if (n == 0) return 0;
    return matches[xorshift32(rng) % (uint32_t)n];
}

static void build_random_blueprint(fd_recipe_t *r, const fd_database_t *db,
                                   const fd_constraints_t *c, uint32_t *rng, int idx_num) {
    memset(r, 0, sizeof(*r));
    snprintf(r->id, sizeof(r->id), "invented_%d", idx_num + 1);
    snprintf(r->category, sizeof(r->category), "invented");

    /* 1. Determine primary structural food or emulsion base */
    int main_idx = -1;
    for (int i = 0; i < c->include_count; i++) {
        int id_idx = fd_db_find_ingredient(db, c->include_ids[i]);
        if (id_idx >= 0 && !is_excluded(c, &db->ingredients[id_idx])) {
            uint32_t rf = db->ingredients[id_idx].role_flags;
            if (main_idx < 0 && (rf & (FD_ROLE_PROTEIN_MAIN | FD_ROLE_VEG_MAIN | FD_ROLE_STARCH_BASE))) {
                main_idx = id_idx;
            }
        }
    }
    if (main_idx < 0) {
        uint32_t main_roles = c->require_emulsion
            ? (FD_ROLE_LIQUID_BASE | FD_ROLE_EMULSIFIER)
            : (FD_ROLE_PROTEIN_MAIN | FD_ROLE_VEG_MAIN | FD_ROLE_STARCH_BASE);
        main_idx = pick_by_role(db, c, main_roles, rng);
        if (main_idx < 0) main_idx = pick_by_role(db, c, 0xFFFFFFFFu, rng);
    }

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

    float max_t = (c->max_time_sec > 90.0f) ? c->max_time_sec : 1500.0f;
    int fat_idx  = pick_by_role(db, c, FD_ROLE_FAT_MEDIUM, rng);
    int salt_idx = pick_by_role(db, c, FD_ROLE_SALT, rng);
    uint32_t accent_role = FD_ROLE_AROMATIC | FD_ROLE_UMAMI;
    if (c->target_sour > 3.0f || c->max_gi > 0.0f) accent_role |= FD_ROLE_ACID;
    if (c->target_sweet > 4.0f) accent_role |= FD_ROLE_SWEETENER;
    int acc_idx = pick_by_role(db, c, accent_role, rng);

    if (c->require_emulsion) {
        /* 2-Vessel Emulsion Workflow: Whisking Bowl + Warm/Drizzle Fat Vessel */
        r->vessel_count = 2;
        snprintf(r->vessels[0].id, sizeof(r->vessels[0].id), "bowl");
        snprintf(r->vessels[0].name, sizeof(r->vessels[0].name), "Whisking Bowl");
        r->vessels[0].kind = FD_VESSEL_BOWL; r->vessels[0].shape = FD_SHAPE_LIQUID;
        r->vessels[0].lx_mm = 80.0f; r->vessels[0].ly_mm = 80.0f; r->vessels[0].lz_mm = 40.0f; r->vessels[0].init_temp_c = 20.0f;

        snprintf(r->vessels[1].id, sizeof(r->vessels[1].id), "fat_pan");
        snprintf(r->vessels[1].name, sizeof(r->vessels[1].name), "Lipid Drizzle Pan");
        r->vessels[1].kind = FD_VESSEL_POT; r->vessels[1].shape = FD_SHAPE_LIQUID;
        r->vessels[1].lx_mm = 65.0f; r->vessels[1].ly_mm = 65.0f; r->vessels[1].lz_mm = 25.0f; r->vessels[1].init_temp_c = 20.0f;

        if (main_idx >= 0) add_unique_item_v(r, db->ingredients[main_idx].id, 55.0f, 0, "aqueous base");
        int em_idx  = pick_by_role(db, c, FD_ROLE_EMULSIFIER, rng);
        int liq_idx = pick_by_role(db, c, FD_ROLE_LIQUID_BASE | FD_ROLE_ACID, rng);
        if (em_idx >= 0)  add_unique_item_v(r, db->ingredients[em_idx].id, 28.0f, 0, "surfactant");
        if (liq_idx >= 0) add_unique_item_v(r, db->ingredients[liq_idx].id, 35.0f, 0, "acid/water phase");
        if (salt_idx >= 0)add_unique_item_v(r, db->ingredients[salt_idx].id, 2.0f, 0, "dissolved");
        if (acc_idx >= 0) add_unique_item_v(r, db->ingredients[acc_idx].id, 14.0f, 0, "finely minced");
        if (fat_idx >= 0) add_unique_item_v(r, db->ingredients[fat_idx].id, 95.0f, 1, "slow thin stream");

        for (int i = 0; i < c->include_count; i++) {
            int id_idx = fd_db_find_ingredient(db, c->include_ids[i]);
            if (id_idx >= 0 && !is_excluded(c, &db->ingredients[id_idx])) {
                add_unique_item_v(r, db->ingredients[id_idx].id, 24.0f, 0, "pantry inclusion");
            }
        }

        float t_em = 58.0f + 9.0f * rand01(rng);
        r->step_count = 3;
        r->steps[0] = (fd_recipe_step_t){ .action = FD_ACT_WHISK, .vessel_idx = 0, .target_vessel_idx = -1,
            .technique_id = "whisk_emulsify", .temp_c = t_em, .duration_sec = 75.0f, .agitation = 0.85f };
        r->steps[1] = (fd_recipe_step_t){ .action = FD_ACT_COOK, .vessel_idx = 1, .target_vessel_idx = -1,
            .technique_id = "simmer_poach", .temp_c = t_em, .duration_sec = 75.0f, .parallel = true };
        r->steps[2] = (fd_recipe_step_t){ .action = FD_ACT_COMBINE, .vessel_idx = 1, .target_vessel_idx = 0,
            .technique_id = "whisk_emulsify", .temp_c = t_em, .duration_sec = 160.0f + 90.0f * rand01(rng), .agitation = 0.92f };
    } else {
        /* 3-Station Culinary Workflow: Prep Board -> Primary Skillet/Pot + Parallel Pan Glaze -> Combine & Rest */
        r->vessel_count = 3;
        snprintf(r->vessels[0].id, sizeof(r->vessels[0].id), "prep");
        snprintf(r->vessels[0].name, sizeof(r->vessels[0].name), "Prep Board");
        r->vessels[0].kind = FD_VESSEL_BOARD; r->vessels[0].shape = r->shape;
        r->vessels[0].lx_mm = r->lx_mm; r->vessels[0].ly_mm = r->ly_mm; r->vessels[0].lz_mm = r->lz_mm; r->vessels[0].init_temp_c = 20.0f;

        snprintf(r->vessels[1].id, sizeof(r->vessels[1].id), "cook");
        snprintf(r->vessels[1].name, sizeof(r->vessels[1].name), "Primary Skillet / Pot");
        r->vessels[1].kind = FD_VESSEL_SKILLET; r->vessels[1].shape = r->shape;
        r->vessels[1].lx_mm = r->lx_mm; r->vessels[1].ly_mm = r->ly_mm; r->vessels[1].lz_mm = r->lz_mm; r->vessels[1].init_temp_c = 20.0f;

        snprintf(r->vessels[2].id, sizeof(r->vessels[2].id), "sauce");
        snprintf(r->vessels[2].name, sizeof(r->vessels[2].name), "Aromatic Saucepan");
        r->vessels[2].kind = FD_VESSEL_POT; r->vessels[2].shape = FD_SHAPE_LIQUID;
        r->vessels[2].lx_mm = 70.0f; r->vessels[2].ly_mm = 70.0f; r->vessels[2].lz_mm = 22.0f; r->vessels[2].init_temp_c = 20.0f;

        if (main_idx >= 0) add_unique_item_v(r, db->ingredients[main_idx].id, 195.0f, 0, "trimmed primary cut");
        if (salt_idx >= 0) add_unique_item_v(r, db->ingredients[salt_idx].id, 2.3f,   0, "dry-brine seasoning");
        if (fat_idx >= 0)  add_unique_item_v(r, db->ingredients[fat_idx].id,  16.0f,  1, "cooking medium");
        if (acc_idx >= 0)  add_unique_item_v(r, db->ingredients[acc_idx].id,  18.0f,  2, "aromatic reduction");
        int liq_idx = pick_by_role(db, c, FD_ROLE_LIQUID_BASE | FD_ROLE_ACID, rng);
        if (liq_idx >= 0)  add_unique_item_v(r, db->ingredients[liq_idx].id,  28.0f,  2, "deglazing liquid");

        for (int i = 0; i < c->include_count; i++) {
            int id_idx = fd_db_find_ingredient(db, c->include_ids[i]);
            if (id_idx >= 0 && !is_excluded(c, &db->ingredients[id_idx])) {
                uint32_t rf = db->ingredients[id_idx].role_flags;
                float def_m = (rf & (FD_ROLE_PROTEIN_MAIN | FD_ROLE_VEG_MAIN | FD_ROLE_STARCH_BASE)) ? 185.0f : 20.0f;
                if (rf & FD_ROLE_SALT) def_m = 2.4f;
                int v_t = (rf & (FD_ROLE_PROTEIN_MAIN | FD_ROLE_VEG_MAIN | FD_ROLE_STARCH_BASE)) ? 0 : 2;
                add_unique_item_v(r, db->ingredients[id_idx].id, def_m, v_t, "pantry inclusion");
            }
        }

        int s = 0;
        /* Step 1: Prep Board sizing / dry-brine */
        r->steps[s++] = (fd_recipe_step_t){
            .action = (r->shape == FD_SHAPE_LIQUID) ? FD_ACT_SALT_BRINE : FD_ACT_CUT_PREP,
            .vessel_idx = 0, .target_vessel_idx = -1, .technique_id = "knife_prep",
            .temp_c = 20.0f, .duration_sec = 30.0f,
            .cut_lx_mm = r->lx_mm, .cut_ly_mm = r->ly_mm, .cut_lz_mm = r->lz_mm,
            .cut_shape = r->shape, .has_cut_shape = true
        };
        /* Step 2: Transfer from Prep Board into Primary Skillet / Pot */
        r->steps[s++] = (fd_recipe_step_t){
            .action = FD_ACT_COMBINE, .vessel_idx = 0, .target_vessel_idx = 1,
            .technique_id = "pan_sear", .temp_c = 22.0f, .duration_sec = 5.0f, .agitation = 0.30f
        };

        /* Step 3..4: 1 or 2 thermal cooking stages on Vessel 1 */
        bool two_cook = (max_t >= 540.0f) && (rand01(rng) > 0.35f);
        int n_cook = two_cook ? 2 : 1;
        float cook_budget = fmaxf(120.0f, max_t - 95.0f);
        for (int k = 0; k < n_cook; k++) {
            int ti = pick_thermal_technique(db, rng);
            if (k == n_cook - 1 && c->target_crispness > 5.0f && fd_db_find_technique(db, "pan_sear") >= 0) {
                ti = fd_db_find_technique(db, "pan_sear");
            } else if (k == 0 && two_cook && fd_db_find_technique(db, "sous_vide") >= 0 && max_t >= 900.0f) {
                ti = fd_db_find_technique(db, "sous_vide");
            }
            const fd_technique_t *tc = &db->techniques[ti];
            fd_recipe_step_t st = { 0 };
            st.action = FD_ACT_COOK;
            st.vessel_idx = 1;
            st.target_vessel_idx = -1;
            snprintf(st.technique_id, sizeof(st.technique_id), "%s", tc->id);
            st.temp_c = clampf(tc->default_temp_c + (rand01(rng) - 0.5f) * 26.0f, tc->min_temp_c, tc->max_temp_c);
            st.duration_sec = clampf((0.30f + 0.45f * rand01(rng)) * (cook_budget / (float)n_cook), 60.0f, cook_budget * 0.72f);
            st.flip_midway = (r->shape != FD_SHAPE_LIQUID);
            st.lid_closed = (k == 0 && two_cook);
            r->steps[s++] = st;
        }

        /* Step 5: Parallel aromatic reduction in Saucepan + Combine & brief carryover Rest */
        if (acc_idx >= 0 || liq_idx >= 0) {
            fd_recipe_step_t s_red = {
                .action = FD_ACT_COOK, .vessel_idx = 2, .target_vessel_idx = -1,
                .temp_c = 92.0f, .duration_sec = 90.0f, .parallel = true
            };
            snprintf(s_red.technique_id, sizeof(s_red.technique_id), "%s",
                     (fd_db_find_technique(db, "deglaze_reduce") >= 0) ? "deglaze_reduce" : "simmer_poach");
            r->steps[s++] = s_red;
            r->steps[s++] = (fd_recipe_step_t){
                .action = FD_ACT_COMBINE, .vessel_idx = 2, .target_vessel_idx = 1,
                .technique_id = "simmer_poach", .temp_c = 68.0f, .duration_sec = 10.0f, .agitation = 0.35f
            };
        }
        fd_recipe_step_t s_rest = {
            .action = FD_ACT_REST, .vessel_idx = 1, .target_vessel_idx = -1,
            .temp_c = 28.0f, .duration_sec = 45.0f
        };
        snprintf(s_rest.technique_id, sizeof(s_rest.technique_id), "%s",
                 (fd_db_find_technique(db, "rest_hold") >= 0) ? "rest_hold" : "ferment_proof");
        r->steps[s++] = s_rest;
        r->step_count = s;
    }

    const char *mname = (main_idx >= 0) ? db->ingredients[main_idx].name : "Culinary Creation";
    const char *aname = (acc_idx >= 0)  ? db->ingredients[acc_idx].name  : "Artisanal";
    snprintf(r->name, sizeof(r->name), "Invented %.26s & %.26s", mname, aname);
    snprintf(r->notes, sizeof(r->notes), "Multi-station workflow synthesized via 3D first-principles evolution.");
}

static void mutate_recipe(fd_recipe_t *r, const fd_database_t *db, const fd_constraints_t *c, uint32_t *rng) {
    for (int i = 0; i < r->item_count; i++) {
        if (rand01(rng) < 0.65f) {
            float scale = 0.78f + 0.44f * rand01(rng);
            r->items[i].mass_g = clampf(r->items[i].mass_g * scale, 0.8f, 450.0f);
        }
    }
    if (r->shape != FD_SHAPE_LIQUID && rand01(rng) < 0.45f) {
        r->lz_mm = clampf(r->lz_mm * (0.82f + 0.36f * rand01(rng)), 8.0f, 65.0f);
        for (int v = 0; v < r->vessel_count; v++) {
            if (r->vessels[v].shape != FD_SHAPE_LIQUID) r->vessels[v].lz_mm = r->lz_mm;
        }
        for (int s = 0; s < r->step_count; s++) {
            if (r->steps[s].action == FD_ACT_CUT_PREP) r->steps[s].cut_lz_mm = r->lz_mm;
        }
    }
    float max_t = (c->max_time_sec > 90.0f) ? c->max_time_sec : 1800.0f;
    for (int s = 0; s < r->step_count; s++) {
        fd_step_action_t act = r->steps[s].action;
        if (act != FD_ACT_COOK && act != FD_ACT_WHISK && !(act == FD_ACT_COMBINE && r->steps[s].duration_sec > 30.0f)) {
            continue;
        }
        int ti = fd_db_find_technique(db, r->steps[s].technique_id);
        float lo = (ti >= 0) ? db->techniques[ti].min_temp_c : 40.0f;
        float hi = (ti >= 0) ? db->techniques[ti].max_temp_c : 240.0f;
        if (rand01(rng) < 0.75f) {
            r->steps[s].temp_c = clampf(r->steps[s].temp_c + (rand01(rng) - 0.5f) * 24.0f, lo, hi);
        }
        if (rand01(rng) < 0.75f) {
            r->steps[s].duration_sec = clampf(r->steps[s].duration_sec * (0.72f + 0.56f * rand01(rng)),
                                              35.0f, max_t * 0.65f);
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
