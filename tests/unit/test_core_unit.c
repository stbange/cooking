#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "fd_app.h"
#include "fd_biology.h"
#include "fd_chemistry.h"
#include "fd_geometry.h"
#include "fd_lua.h"
#include "fd_nutrition.h"
#include "fd_physics.h"
#include "fd_sim.h"
#include "fd_solver.h"
#include "fd_ui.h"
#include "fd_vfs.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_vfs_module(void) {
    assert(!fd_vfs_valid_relpath(NULL));
    assert(!fd_vfs_valid_relpath(""));
    assert(!fd_vfs_valid_relpath("/etc/passwd"));
    assert(!fd_vfs_valid_relpath("scripts\\init.lua"));
    assert(!fd_vfs_valid_relpath("scripts/../secret.txt"));
    assert(fd_vfs_valid_relpath("scripts/init.lua"));

    char long_path[200];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    assert(!fd_vfs_valid_relpath(long_path));

    /* Manifest parsing including escaped chars & traversal rejection */
    const char *mjson = "[\"scripts/init.lua\", \"../bad.lua\", \"fonts/fatduck_font.png\", \"a\\\\b\"]";
    char paths[8][FD_VFS_MAX_PATH];
    assert(fd_vfs_parse_manifest(NULL, 0, paths, 8) == 0);
    int n = fd_vfs_parse_manifest(mjson, strlen(mjson), paths, 8);
    assert(n == 2);
    assert(strcmp(paths[0], "scripts/init.lua") == 0);
    assert(strcmp(paths[1], "fonts/fatduck_font.png") == 0);

    /* Init path resolution branches */
    fd_vfs_t vfs;
    fd_vfs_init(&vfs, "assets", NULL);
    assert(strcmp(vfs.base_dir, "assets") == 0);

    setenv("FATDUCK_ASSETS_DIR", "assets", 1);
    fd_vfs_init(&vfs, NULL, NULL);
    assert(strcmp(vfs.base_dir, "assets") == 0);
    unsetenv("FATDUCK_ASSETS_DIR");

    fd_vfs_init(&vfs, NULL, "./");
    assert(strstr(vfs.base_dir, "assets") != NULL);

    fd_vfs_init(&vfs, NULL, "/nonexistent/dir/");
    assert(strstr(vfs.base_dir, "assets") != NULL);

    /* Disk read + missing file + override lifecycle */
    uint8_t *buf = NULL;
    size_t sz = 0;
    assert(!fd_vfs_read(&vfs, "../bad", &buf, &sz));
    assert(!fd_vfs_read(&vfs, "does_not_exist.txt", &buf, &sz));
    assert(fd_vfs_read(&vfs, "scripts/init.lua", &buf, &sz));
    assert(sz > 20 && buf != NULL);
    free(buf);

    /* Put base vs override precedence */
    assert(!fd_vfs_put(&vfs, "../bad", "x", 1, false));
    assert(fd_vfs_put(&vfs, "scripts/custom.lua", "base_v", 6, false));
    assert(fd_vfs_put(&vfs, "scripts/custom.lua", "over_v", 6, true));
    /* Base refresh should NOT clobber active override */
    assert(fd_vfs_put(&vfs, "scripts/custom.lua", "base_2", 6, false));
    assert(fd_vfs_read(&vfs, "scripts/custom.lua", &buf, &sz));
    assert(strcmp((char *)buf, "over_v") == 0);
    free(buf);

    assert(fd_vfs_clear_override(&vfs, "scripts/custom.lua"));
    assert(!fd_vfs_clear_override(&vfs, "scripts/custom.lua"));
    fd_vfs_free(&vfs);
}

static void test_lua_sandbox_and_errors(void) {
    fd_vfs_t vfs;
    fd_database_t db;
    fd_lua_t lua;
    memset(&db, 0, sizeof(db));
    fd_vfs_init(&vfs, "assets", NULL);
    assert(!fd_lua_init(NULL, &vfs, &db));
    assert(fd_lua_init(&lua, &vfs, &db));

    /* Verify os, io, dofile, loadfile are stripped */
    assert(fd_lua_exec_str(&lua, "chk",
        "assert(os == nil and io == nil and package == nil and debug == nil and dofile == nil and loadfile == nil)",
        10000));

    /* Syntax error & runtime error */
    assert(!fd_lua_exec_str(&lua, "bad_syn", "function (( broken", 10000));
    assert(strlen(lua.last_error) > 0);

    /* Infinite loop caught by instruction hook */
    assert(!fd_lua_exec_str(&lua, "inf_loop", "while true do end", 8000));
    assert(strstr(lua.last_error, "quota") != NULL);

    /* Memory bomb caught by sandbox allocator */
    assert(!fd_lua_exec_str(&lua, "mem_bomb",
        "local s = 'aaaaaaaaaa'; for i=1,30 do s = s .. s end", 500000));

    /* Validation errors in registration API */
    assert(!fd_lua_exec_str(&lua, "no_id", "fatduck.add_ingredient({})", 10000));
    assert(!fd_lua_exec_str(&lua, "no_tid", "fatduck.add_technique({})", 10000));
    assert(!fd_lua_exec_str(&lua, "no_rid", "fatduck.add_recipe({})", 10000));
    assert(!fd_lua_exec_str(&lua, "bad_inc", "fatduck.include('../secret.lua')", 10000));
    assert(!fd_lua_exec_str(&lua, "miss_inc", "fatduck.include('scripts/missing_xyz.lua')", 10000));
    assert(!fd_lua_exec_file(&lua, "../bad.lua", 10000));
    assert(!fd_lua_exec_file(&lua, "scripts/missing_xyz.lua", 10000));

    /* Reload full database & test duplicate updates + mod hooks */
    assert(fd_lua_reload_all(&lua));
    assert(db.ingredient_count >= 105);
    assert(db.technique_count == 15);
    assert(db.recipe_count >= 32);

    /* Re-register existing id updates in place */
    int prev_cnt = db.ingredient_count;
    assert(fd_lua_exec_str(&lua, "upd",
        "fatduck.add_ingredient({ id='sea_salt', name='Flaky Sea Salt', ash=0.999, sodium_mg_100g=38758, shape='liquid', diet={'vegan','vegetarian','gluten_free','dairy_free'}, roles={'salt'} })",
        20000));
    assert(db.ingredient_count == prev_cnt);
    assert(fd_db_find_ingredient(&db, "nonexistent_xyz") == -1);
    assert(fd_db_find_technique(&db, "nonexistent_xyz") == -1);
    assert(fd_db_find_recipe(&db, "nonexistent_xyz") == -1);

    fd_metrics_t m = { .overall_score = 75.0f, .crispness = 7.0f, .tenderness = 8.0f, .umami = 7.5f };
    fd_lua_call_on_step(&lua, 12.0, 52.0f, 165.0f);
    assert(fd_lua_call_on_score(&lua, &m) > 0.0f);

    fd_lua_free(&lua);
    fd_vfs_free(&vfs);
}

static void test_geometry_all_shapes_and_slices(void) {
    fd_shape_type_t shapes[9] = {
        FD_SHAPE_LIQUID, FD_SHAPE_SLAB, FD_SHAPE_CUBE, FD_SHAPE_CYLINDER,
        FD_SHAPE_SPHERE, FD_SHAPE_TAPERED_FILLET, FD_SHAPE_FAT_CAP_SLAB,
        FD_SHAPE_BONE_IN_SLAB, FD_SHAPE_CONCENTRIC_EGG
    };
    uint8_t rgba[FD_MAX_GRID_DIM * FD_MAX_GRID_DIM * 4];

    for (int s = 0; s < 9; s++) {
        int nx = 0, ny = 0, nz = 0;
        fd_geometry_choose_dims(shapes[s], 100.0f, 70.0f, (s == 1) ? 20.0f : 70.0f, (s % 2 == 0), &nx, &ny, &nz);
        fd_grid_t g;
        memset(&g, 0, sizeof(g));
        assert(fd_grid_alloc(&g, nx, ny, nz, 100.0f, 70.0f, 35.0f));
        fd_grid_rasterize(&g, shapes[s], (s == 2) ? -5.0f : 20.0f, 0.72f, 0.12f, 0.01f, 1.25f);
        assert(g.active_count > 0);

        /* Test high temp & char coloring branches */
        g.T[0] = 175.0f;
        g.char_level[0] = 0.35f;

        fd_grid_flip_z(&g);
        fd_grid_toss_cubes(&g);

        for (int pl = 0; pl < 3; pl++) {
            for (int ov = 0; ov < 5; ov++) {
                int w = 0, h = 0;
                fd_grid_extract_slice(&g, (fd_slice_plane_t)pl, 2, (fd_overlay_mode_t)ov, rgba, &w, &h);
                assert(w > 0 && h > 0);
            }
        }
        fd_grid_free(&g);
    }
}

static void test_physics_chemistry_biology_edge_cases(void) {
    float rho = 0.0f, cp = 0.0f, kc = 0.0f;
    /* Frozen fusion peak (-1.2°C), room temp, and porous baked crumb */
    fd_choi_okos(-1.2f, 0.35f, 0.35f, 0.20f, 0.08f, 0.01f, 0.0f, 0.01f, 0.0f, &rho, &cp, &kc);
    assert(cp > 15000.0f && rho > 900.0f && kc > 0.4f);

    fd_choi_okos(95.0f, 0.35f, 0.0f, 0.12f, 0.05f, 0.45f, 0.02f, 0.01f, 0.65f, &rho, &cp, &kc);
    assert(rho < 600.0f && kc < 0.35f);

    assert(fd_compute_water_activity(0.0f, 0.0f, 0.0f) < 0.1f);
    assert(fd_compute_water_activity(0.10f, 0.08f, 0.20f) < 0.5f);
    assert(fd_compute_water_activity(0.85f, 0.005f, 0.01f) > 0.95f);

    assert(fd_cardinal_temp_activity(0.0f, 5.0f, 30.0f, 45.0f) == 0.0f);
    assert(fd_cardinal_temp_activity(50.0f, 5.0f, 30.0f, 45.0f) == 0.0f);
    assert(fd_cardinal_temp_activity(30.0f, 5.0f, 30.0f, 45.0f) > 0.95f);

    /* Emulsion split under extreme heat & acid + oil smoke point */
    fd_vfs_t vfs; fd_database_t db; fd_lua_t lua;
    memset(&db, 0, sizeof(db));
    fd_vfs_init(&vfs, "assets", NULL);
    fd_lua_init(&lua, &vfs, &db);
    fd_lua_reload_all(&lua);

    fd_sim_state_t sim;
    memset(&sim, 0, sizeof(sim));
    int holl = fd_db_find_recipe(&db, "classic_hollandaise");
    assert(holl >= 0);
    fd_recipe_t broken = db.recipes[holl];
    broken.lz_mm = 8.0f;
    broken.steps[0].temp_c = 98.0f; /* Overheat thin yolk film on boiling bath -> curdle! */
    broken.steps[0].duration_sec = 360.0f;
    assert(fd_sim_run_recipe(&sim, &db, &lua, &broken, true));
    assert(sim.metrics.emulsion_stability < 0.6f);
    fd_sim_free(&sim);

    /* Burn & smoke point trigger at extreme grill heat */
    int rib = fd_db_find_recipe(&db, "reverse_sear_ribeye");
    assert(rib >= 0);
    fd_recipe_t burnt = db.recipes[rib];
    burnt.items[1].ingredient_id[0] = '\0';
    snprintf(burnt.items[1].ingredient_id, sizeof(burnt.items[1].ingredient_id), "butter"); /* 175°C smoke point */
    burnt.steps[1].technique_id[0] = '\0';
    snprintf(burnt.steps[1].technique_id, sizeof(burnt.steps[1].technique_id), "broil_grill");
    burnt.steps[1].temp_c = 285.0f;
    burnt.steps[1].duration_sec = 420.0f;
    assert(fd_sim_run_recipe(&sim, &db, &lua, &burnt, true));
    assert(sim.metrics.oil_smoked);
    assert(sim.metrics.max_char > 0.05f);
    fd_sim_free(&sim);

    /* Sweet potato amylase conversion at ~65°C */
    fd_recipe_item_t sp_it[2] = { { "sweet_potato", 200.0f }, { "water", 40.0f } };
    assert(fd_sim_init_dish(&sim, &db, sp_it, 2, FD_SHAPE_CYLINDER, 80.0f, 45.0f, 45.0f, 62.0f, true));
    float sug_before = sim.dry_sugar_g;
    fd_sim_set_technique(&sim, &db, "sous_vide", 66.0f, true);
    fd_sim_step(&sim, &db, &lua, 900.0f);
    assert(sim.dry_sugar_g > sug_before);
    fd_sim_free(&sim);

    fd_lua_free(&lua);
    fd_vfs_free(&vfs);
}

static void test_solver_propose_and_invent(void) {
    fd_vfs_t vfs; fd_database_t db; fd_lua_t lua;
    memset(&db, 0, sizeof(db));
    fd_vfs_init(&vfs, "assets", NULL);
    fd_lua_init(&lua, &vfs, &db);
    fd_lua_reload_all(&lua);

    fd_constraints_t c;
    fd_constraints_init_default(&c);
    c.required_diet_flags = FD_DIET_VEGAN | FD_DIET_GLUTEN_FREE;
    c.max_gi = 58.0f;
    snprintf(c.exclude_ids[0], sizeof(c.exclude_ids[0]), "butter");
    c.exclude_count = 1;

    fd_proposal_t props[4];
    int np = fd_solver_propose_classics(&db, &lua, &c, props, 4);
    assert(np > 0);
    assert(props[0].metrics.is_pasteurized_safe);

    /* Invent vegan emulsion recipe */
    fd_constraints_init_default(&c);
    c.required_diet_flags = FD_DIET_VEGAN;
    c.require_emulsion    = true;
    c.target_sour         = 4.0f;
    c.rng_seed            = 12345u;

    fd_inventor_t inv;
    assert(fd_inventor_start(&inv, &db, &c));
    assert(fd_inventor_progress(&inv) < 0.05f);
    while (!fd_inventor_step(&inv, &db, &lua, 18)) {}
    assert(fd_inventor_progress(&inv) == 1.0f);
    assert(inv.best.recipe.item_count >= 2);
    assert(inv.best.metrics.is_pasteurized_safe);

    /* Invent high-protein crisp meat/seafood dish with mandatory salmon */
    fd_constraints_init_default(&c);
    snprintf(c.include_ids[0], sizeof(c.include_ids[0]), "salmon_fillet");
    c.include_count     = 1;
    c.min_protein_g     = 25.0f;
    c.target_crispness  = 7.0f;
    c.target_tenderness = 7.5f;
    c.target_umami      = 6.5f;
    c.target_sweet      = 1.5f;
    c.max_kcal_per_100g = 260.0f;
    c.max_time_sec      = 900.0f;
    assert(fd_inventor_start(&inv, &db, &c));
    assert(fd_inventor_step(&inv, &db, &lua, 100));
    assert(inv.done);
    assert(inv.best.metrics.protein_g >= 20.0f);

    fd_lua_free(&lua);
    fd_vfs_free(&vfs);
}

int main(void) {
    test_vfs_module();
    test_lua_sandbox_and_errors();
    test_geometry_all_shapes_and_slices();
    test_physics_chemistry_biology_edge_cases();
    test_solver_propose_and_invent();
    printf("ALL CORE UNIT TESTS PASSED.\n");
    return 0;
}
