#include "fd_biology.h"
#include "fd_chemistry.h"
#include "fd_geometry.h"
#include "fd_lua.h"
#include "fd_nutrition.h"
#include "fd_physics.h"
#include "fd_sim.h"
#include "fd_vfs.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void verify_3d_corner_vs_edge_vs_face_physics(const fd_database_t *db) {
    /* Prove true 3D spatial accuracy: 3-axis corner heats & browns faster than 2-axis edge > 1-axis face > 3D center! */
    fd_sim_state_t sim;
    memset(&sim, 0, sizeof(sim));
    fd_recipe_item_t items[1] = { { "firm_tofu", 180.0f } };
    assert(fd_sim_init_dish(&sim, db, items, 1, FD_SHAPE_CUBE, 40.0f, 40.0f, 40.0f, 20.0f, false));
    fd_sim_set_technique(&sim, db, "convection_roast", 210.0f, false);

    for (int i = 0; i < 12; i++) {
        fd_sim_step(&sim, db, NULL, 15.0f);
    }

    const fd_grid_t *g = &sim.grid;
    int mid = g->nx / 2;
    float t_corner = g->T[fd_grid_idx(g, 0, 0, g->nz - 1)];          /* 3 exposed faces (+Z, -X, -Y) */
    float t_edge   = g->T[fd_grid_idx(g, mid, 0, g->nz - 1)];        /* 2 exposed faces (+Z, -Y) */
    float t_face   = g->T[fd_grid_idx(g, mid, mid, g->nz - 1)];      /* 1 exposed face (+Z) */
    float t_center = g->T[fd_grid_idx(g, mid, mid, mid)];            /* 0 exposed faces (3D core) */

    printf("[3D Cube Verification] Corner=%.2f°C > Edge=%.2f°C > Face=%.2f°C > Core=%.2f°C\n",
           t_corner, t_edge, t_face, t_center);
    assert(t_corner > t_edge + 0.5f);
    assert(t_edge   > t_face + 0.5f);
    assert(t_face   > t_center + 5.0f);
    fd_sim_free(&sim);
}

static void verify_tapered_fillet_asymmetric_cold_spot(const fd_database_t *db) {
    fd_sim_state_t sim;
    memset(&sim, 0, sizeof(sim));
    fd_recipe_item_t items[1] = { { "chicken_breast", 210.0f } };
    assert(fd_sim_init_dish(&sim, db, items, 1, FD_SHAPE_TAPERED_FILLET, 125.0f, 75.0f, 30.0f, 15.0f, false));
    fd_sim_set_technique(&sim, db, "convection_roast", 185.0f, false);
    for (int i = 0; i < 15; i++) fd_sim_step(&sim, db, NULL, 20.0f);

    /* Cold spot must lie in the thick head half (x < nx/2), NOT the thin tail! */
    printf("[3D Tapered Fillet] Cold spot at x=%d/%d (%.1f°C)\n",
           sim.metrics.cold_x, sim.grid.nx, sim.metrics.cold_spot_temp_c);
    assert(sim.metrics.cold_x < sim.grid.nx / 2);
    fd_sim_free(&sim);
}

static void verify_rs3_glycemic_retrogradation_and_braise_collagen(const fd_database_t *db, fd_lua_t *lua) {
    /* Compare hot boiled potato puree vs chilled vinaigrette RS3 potato salad */
    fd_sim_state_t hot_pot, cold_pot;
    memset(&hot_pot, 0, sizeof(hot_pot));
    memset(&cold_pot, 0, sizeof(cold_pot));

    int idx_puree = fd_db_find_recipe(db, "pommes_puree");
    int idx_rs3   = fd_db_find_recipe(db, "chilled_potato_salad_rs3");
    assert(idx_puree >= 0 && idx_rs3 >= 0);

    assert(fd_sim_run_recipe(&hot_pot,  db, lua, &db->recipes[idx_puree], true));
    assert(fd_sim_run_recipe(&cold_pot, db, lua, &db->recipes[idx_rs3],   true));

    printf("[Glycemic RS3 Verification] Hot Potato RS=%.2fg (eGI=%.1f) vs Chilled RS3=%.2fg (eGI=%.1f)\n",
           hot_pot.metrics.resistant_starch_g, hot_pot.metrics.estimated_gi,
           cold_pot.metrics.resistant_starch_g, cold_pot.metrics.estimated_gi);
    assert(cold_pot.metrics.resistant_starch_g > hot_pot.metrics.resistant_starch_g + 1.0f);
    assert(cold_pot.metrics.estimated_gi < hot_pot.metrics.estimated_gi - 4.0f);

    fd_sim_free(&hot_pot);
    fd_sim_free(&cold_pot);

    /* Verify slow collagen braise converts tough chuck into gelatin */
    fd_sim_state_t braise;
    memset(&braise, 0, sizeof(braise));
    int idx_bb = fd_db_find_recipe(db, "boeuf_bourguignon");
    assert(idx_bb >= 0);
    assert(fd_sim_run_recipe(&braise, db, lua, &db->recipes[idx_bb], true));
    printf("[Collagen Hydrolysis] Boeuf Bourguignon Gelatin=%.1f%%, Tenderness=%.2f/10, Safe=%d\n",
           braise.metrics.gelatin_converted * 100.0f, braise.metrics.tenderness,
           braise.metrics.is_pasteurized_safe ? 1 : 0);
    assert(braise.metrics.gelatin_converted > 0.45f);
    assert(braise.metrics.is_pasteurized_safe);
    fd_sim_free(&braise);
}

static void verify_movement_and_multivessel_physics(const fd_database_t *db, fd_lua_t *lua) {
    /* 1. Knife Cut L^2 Conduction & Cell Rupture: 12mm dice vs 42mm chunk */
    fd_sim_state_t fine_cut, coarse_cut;
    memset(&fine_cut, 0, sizeof(fine_cut));
    memset(&coarse_cut, 0, sizeof(coarse_cut));
    fd_recipe_item_t veg[2] = { { .ingredient_id = "carrot", .mass_g = 150.0f }, { .ingredient_id = "garlic", .mass_g = 15.0f } };
    assert(fd_sim_init_dish(&fine_cut,   db, veg, 2, FD_SHAPE_CUBE, 42.0f, 42.0f, 42.0f, 20.0f, true));
    assert(fd_sim_init_dish(&coarse_cut, db, veg, 2, FD_SHAPE_CUBE, 42.0f, 42.0f, 42.0f, 20.0f, true));
    assert(fd_sim_apply_action(&fine_cut, db, lua, FD_ACT_CUT_PREP, 0.8f, 12.0f, 12.0f, 12.0f, FD_SHAPE_CUBE, true, 20.0f, 30.0f, true));
    fd_sim_set_technique(&fine_cut,   db, "saute_stirfry", 185.0f, false);
    fd_sim_set_technique(&coarse_cut, db, "saute_stirfry", 185.0f, false);
    fd_sim_step(&fine_cut,   db, lua, 180.0f);
    fd_sim_step(&coarse_cut, db, lua, 180.0f);
    printf("[Knife Prep L^2] 12mm Core=%.1f°C Aroma=%.2f vs 42mm Core=%.1f°C Aroma=%.2f\n",
           fine_cut.metrics.core_temp_c, fine_cut.metrics.aroma,
           coarse_cut.metrics.core_temp_c, coarse_cut.metrics.aroma);
    assert(fine_cut.metrics.core_temp_c > coarse_cut.metrics.core_temp_c + 12.0f);
    assert(fine_cut.metrics.aroma > coarse_cut.metrics.aroma);
    fd_sim_free(&fine_cut);
    fd_sim_free(&coarse_cut);

    /* 2. Gentle Folding vs Harsh Stirring on Whipped Chocolate Fondant Foam */
    int idx_fond = fd_db_find_recipe(db, "chocolate_fondant");
    assert(idx_fond >= 0);
    fd_recipe_t folded = db->recipes[idx_fond];
    fd_recipe_t harsh  = db->recipes[idx_fond];
    for (int s = 0; s < harsh.step_count; s++) {
        if (harsh.steps[s].action == FD_ACT_FOLD_STIR || harsh.steps[s].action == FD_ACT_COMBINE) {
            harsh.steps[s].agitation = 0.95f; /* Over-mix with high shear! */
        }
    }
    fd_sim_state_t s_fold, s_harsh;
    memset(&s_fold, 0, sizeof(s_fold));
    memset(&s_harsh, 0, sizeof(s_harsh));
    assert(fd_sim_run_recipe(&s_fold,  db, lua, &folded, true));
    assert(fd_sim_run_recipe(&s_harsh, db, lua, &harsh,  true));
    printf("[Foam Folding] Gentle Fold Porosity=%.1f%% vs Harsh Stir Porosity=%.1f%%\n",
           s_fold.metrics.avg_porosity * 100.0f, s_harsh.metrics.avg_porosity * 100.0f);
    assert(s_fold.metrics.avg_porosity > s_harsh.metrics.avg_porosity + 0.08f);
    fd_sim_free(&s_fold);
    fd_sim_free(&s_harsh);

    /* 3. Multi-Vessel Roman Carbonara: 65°C off-heat tempering vs 95°C scrambled skillet */
    int idx_carb = fd_db_find_recipe(db, "pasta_carbonara");
    assert(idx_carb >= 0);
    fd_recipe_t good_carb = db->recipes[idx_carb];
    fd_recipe_t bad_carb  = db->recipes[idx_carb];
    bad_carb.steps[bad_carb.step_count - 1].temp_c = 96.0f; /* Dump yolks over blazing burner! */
    fd_sim_state_t s_good, s_bad;
    memset(&s_good, 0, sizeof(s_good));
    memset(&s_bad, 0, sizeof(s_bad));
    assert(fd_sim_run_recipe(&s_good, db, lua, &good_carb, true));
    assert(fd_sim_run_recipe(&s_bad,  db, lua, &bad_carb,  true));
    printf("[Carbonara Multi-Vessel] 65°C Temper Emulsion=%.0f%% (Safe=%d) vs 96°C Scramble=%.0f%%\n",
           s_good.metrics.emulsion_stability * 100.0f, s_good.metrics.is_pasteurized_safe ? 1 : 0,
           s_bad.metrics.emulsion_stability * 100.0f);
    assert(s_good.metrics.is_pasteurized_safe);
    assert(s_good.metrics.emulsion_stability > s_bad.metrics.emulsion_stability + 0.15f);
    fd_sim_free(&s_good);
    fd_sim_free(&s_bad);
}

static void verify_all_classic_recipes(const fd_database_t *db, fd_lua_t *lua) {
    printf("=== Simulating all %d multi-station recipes in ./assets/scripts/recipes/ ===\n", db->recipe_count);
    assert(db->recipe_count >= 34);
    int bad = 0;
    for (int i = 0; i < db->recipe_count; i++) {
        fd_sim_state_t sim;
        memset(&sim, 0, sizeof(sim));
        const fd_recipe_t *r = &db->recipes[i];
        assert(fd_sim_run_recipe(&sim, db, lua, r, true));
        printf("  [%02d] %-36s (%dv/%ds) Score=%5.1f Cold=%.1fC(%.1fD) Surf=%.1fC Char=%.2f Safe=%d\n",
               i + 1, r->id, r->vessel_count, r->step_count,
               sim.metrics.overall_score, sim.metrics.cold_spot_temp_c,
               sim.metrics.cold_spot_lethality_log10, sim.metrics.surface_max_temp_c,
               sim.metrics.max_char, sim.metrics.is_pasteurized_safe ? 1 : 0);
        if (isnan(sim.metrics.overall_score) || isnan(sim.metrics.kcal_total) ||
            sim.metrics.overall_score < 40.0f || sim.metrics.max_char >= 0.35f) {
            bad++;
        }
        fd_sim_free(&sim);
    }
    fflush(stdout);
    assert(bad == 0);
}

int main(void) {
    fd_vfs_t vfs;
    fd_database_t db;
    fd_lua_t lua;
    memset(&db, 0, sizeof(db));
    fd_vfs_init(&vfs, "assets", NULL);
    assert(fd_lua_init(&lua, &vfs, &db));
    assert(fd_lua_reload_all(&lua));

    verify_3d_corner_vs_edge_vs_face_physics(&db);
    verify_tapered_fillet_asymmetric_cold_spot(&db);
    verify_rs3_glycemic_retrogradation_and_braise_collagen(&db, &lua);
    verify_movement_and_multivessel_physics(&db, &lua);
    verify_all_classic_recipes(&db, &lua);

    fd_lua_free(&lua);
    fd_vfs_free(&vfs);
    printf("ALL 3D FIRST-PRINCIPLES & MULTI-VESSEL SCIENCE BENCHMARKS PASSED.\n");
    return 0;
}
