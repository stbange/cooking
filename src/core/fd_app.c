#include "fd_app.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void refresh_slice(fd_app_t *app) {
    if (!app->sim.initialized) return;
    const fd_grid_t *g = &app->sim.grid;
    int max_idx = (app->slice_plane == FD_SLICE_XY) ? (g->nz - 1)
                : ((app->slice_plane == FD_SLICE_XZ) ? (g->ny - 1) : (g->nx - 1));
    int sidx = (int)roundf(clampf(app->slice_norm, 0.0f, 1.0f) * (float)max_idx);
    fd_grid_extract_slice(g, app->slice_plane, sidx, app->overlay_mode,
                          app->slice_rgba, &app->slice_w, &app->slice_h);
    app->slice_dirty = true;
}

bool fd_app_load_recipe(fd_app_t *app, const fd_recipe_t *r) {
    if (!app || !r || r->item_count <= 0) return false;
    if (!fd_sim_init_dish(&app->sim, &app->db, r->items, r->item_count,
                          r->shape, r->lx_mm, r->ly_mm, r->lz_mm, 20.0f, false)) {
        return false;
    }
    if (r->step_count > 0) {
        fd_sim_set_technique(&app->sim, &app->db, r->steps[0].technique_id,
                             r->steps[0].temp_c, r->steps[0].lid_closed);
    }
    app->slice_plane = FD_SLICE_XZ;
    app->slice_norm  = 0.5f;
    refresh_slice(app);
    app->pending_cue = FD_CUE_SEAR_DROP;
    snprintf(app->status_msg, sizeof(app->status_msg), "Loaded: %s (%dx%dx%d 3D grid)",
             r->name, app->sim.grid.nx, app->sim.grid.ny, app->sim.grid.nz);
    return true;
}

bool fd_app_load_recipe_index(fd_app_t *app, int recipe_idx) {
    if (!app || recipe_idx < 0 || recipe_idx >= app->db.recipe_count) return false;
    app->selected_recipe_idx = recipe_idx;
    return fd_app_load_recipe(app, &app->db.recipes[recipe_idx]);
}

bool fd_app_init(fd_app_t *app, const char *explicit_assets_dir, const char *exe_base_path) {
    if (!app) return false;
    memset(app, 0, sizeof(*app));
    fd_vfs_init(&app->vfs, explicit_assets_dir, exe_base_path);
    if (!fd_lua_init(&app->lua, &app->vfs, &app->db)) return false;
    if (!fd_lua_exec_file(&app->lua, "scripts/init.lua", 2000000)) {
        snprintf(app->status_msg, sizeof(app->status_msg), "Lua error: %.120s", app->lua.last_error);
        return false;
    }

    app->time_scale   = 5.0f;
    app->paused       = false;
    app->slice_plane  = FD_SLICE_XZ;
    app->slice_norm   = 0.5f;
    app->overlay_mode = FD_OVERLAY_TEMP;
    fd_constraints_init_default(&app->constraints);

    if (app->db.recipe_count > 0) {
        fd_app_load_recipe_index(app, 0);
    }
    snprintf(app->status_msg, sizeof(app->status_msg),
             "Ready: %d ingredients, %d techniques, %d recipes loaded from ./assets/",
             app->db.ingredient_count, app->db.technique_count, app->db.recipe_count);
    return true;
}

void fd_app_free(fd_app_t *app) {
    if (!app) return;
    fd_sim_free(&app->sim);
    fd_lua_free(&app->lua);
    fd_vfs_free(&app->vfs);
}

void fd_app_snap_cold_spot(fd_app_t *app) {
    if (!app || !app->sim.initialized) return;
    const fd_grid_t *g = &app->sim.grid;
    const fd_metrics_t *m = &app->sim.metrics;
    if (app->slice_plane == FD_SLICE_XY)      app->slice_norm = (g->nz > 1) ? ((float)m->cold_z / (float)(g->nz - 1)) : 0.5f;
    else if (app->slice_plane == FD_SLICE_XZ) app->slice_norm = (g->ny > 1) ? ((float)m->cold_y / (float)(g->ny - 1)) : 0.5f;
    else                                      app->slice_norm = (g->nx > 1) ? ((float)m->cold_x / (float)(g->nx - 1)) : 0.5f;
    refresh_slice(app);
    snprintf(app->status_msg, sizeof(app->status_msg), "Snapped to 3D Cold Spot (%d,%d,%d): %.1f°C | %.2f log10",
             m->cold_x, m->cold_y, m->cold_z, m->cold_spot_temp_c, m->cold_spot_lethality_log10);
}

void fd_app_snap_hot_spot(fd_app_t *app) {
    if (!app || !app->sim.initialized) return;
    const fd_grid_t *g = &app->sim.grid;
    const fd_metrics_t *m = &app->sim.metrics;
    if (app->slice_plane == FD_SLICE_XY)      app->slice_norm = (g->nz > 1) ? ((float)m->hot_z / (float)(g->nz - 1)) : 0.5f;
    else if (app->slice_plane == FD_SLICE_XZ) app->slice_norm = (g->ny > 1) ? ((float)m->hot_y / (float)(g->ny - 1)) : 0.5f;
    else                                      app->slice_norm = (g->nx > 1) ? ((float)m->hot_x / (float)(g->nx - 1)) : 0.5f;
    refresh_slice(app);
    snprintf(app->status_msg, sizeof(app->status_msg), "Snapped to 3D Peak Crust/Corner (%d,%d,%d): %.1f°C | Char %.0f%%",
             m->hot_x, m->hot_y, m->hot_z, m->surface_max_temp_c, m->max_char * 100.0f);
}

static void draw_telemetry_plot(fd_app_t *app, float x, float y, float w, float h) {
    fd_ui_t *ui = &app->ui;
    fd_ui_rect(ui, x, y, w, h, (fd_color_t){ 18, 22, 30, 255 });
    fd_ui_outline(ui, x, y, w, h, 1.0f, (fd_color_t){ 62, 74, 94, 255 });
    fd_ui_text(ui, x + 8.0f, y + 6.0f, 0.85f, (fd_color_t){ 210, 220, 235, 255 },
               "3D Thermal & Maillard Curves (Surf °C, Cold-Spot °C, Browning)");

    int n = app->sim.hist_count;
    if (n < 2) return;
    int start = (app->sim.hist_count == FD_HISTORY_LEN) ? app->sim.hist_head : 0;
    float px = x + 8.0f, py = y + 26.0f, pw = w - 16.0f, ph = h - 34.0f;

    for (int i = 0; i < n - 1; i++) {
        int i0 = (start + i) % FD_HISTORY_LEN;
        int i1 = (start + i + 1) % FD_HISTORY_LEN;
        float x0 = px + ((float)i / (float)(n - 1)) * pw;
        float x1 = px + ((float)(i + 1) / (float)(n - 1)) * pw;

        float ys0 = py + ph * (1.0f - clampf(app->sim.hist_t_surf[i0] / 250.0f, 0.0f, 1.0f));
        float ys1 = py + ph * (1.0f - clampf(app->sim.hist_t_surf[i1] / 250.0f, 0.0f, 1.0f));
        fd_ui_line(ui, x0, ys0, x1, ys1, 2.0f, (fd_color_t){ 245, 132, 58, 255 });

        float yc0 = py + ph * (1.0f - clampf(app->sim.hist_t_cold[i0] / 250.0f, 0.0f, 1.0f));
        float yc1 = py + ph * (1.0f - clampf(app->sim.hist_t_cold[i1] / 250.0f, 0.0f, 1.0f));
        fd_ui_line(ui, x0, yc0, x1, yc1, 2.0f, (fd_color_t){ 72, 168, 245, 255 });

        float ym0 = py + ph * (1.0f - clampf(app->sim.hist_maillard[i0], 0.0f, 1.0f));
        float ym1 = py + ph * (1.0f - clampf(app->sim.hist_maillard[i1], 0.0f, 1.0f));
        fd_ui_line(ui, x0, ym0, x1, ym1, 1.8f, (fd_color_t){ 238, 205, 82, 255 });
    }
}

static void draw_metrics_card(fd_app_t *app, float x, float y, float w, float h) {
    fd_ui_t *ui = &app->ui;
    const fd_metrics_t *m = &app->sim.metrics;
    fd_ui_rect(ui, x, y, w, h, (fd_color_t){ 24, 29, 39, 255 });
    fd_ui_outline(ui, x, y, w, h, 1.0f, (fd_color_t){ 62, 74, 94, 255 });

    char line[128];
    float cy = y + 8.0f;
    snprintf(line, sizeof(line), "Time: %.0fs | Score: %.1f/100 | %s",
             app->sim.elapsed_sec, m->overall_score,
             m->is_pasteurized_safe ? "SAFE (>=6.5D)" : "RAW / UNPASTEURIZED");
    fd_ui_text(ui, x + 8.0f, cy, 0.90f,
               m->is_pasteurized_safe ? (fd_color_t){ 82, 224, 142, 255 } : (fd_color_t){ 248, 108, 96, 255 }, line);
    cy += 19.0f;

    snprintf(line, sizeof(line), "3D Surf: %.1f°C | ColdSpot: %.1f°C (%.1fD) | pH %.2f",
             m->surface_max_temp_c, m->cold_spot_temp_c, m->cold_spot_lethality_log10, m->ph);
    fd_ui_text(ui, x + 8.0f, cy, 0.86f, (fd_color_t){ 228, 234, 245, 255 }, line);
    cy += 18.0f;

    snprintf(line, sizeof(line), "Maillard %.0f%% | Char %.0f%% | Gelatin %.0f%% | StarchGel %.0f%%",
             m->crust_maillard * 100.0f, m->max_char * 100.0f,
             m->gelatin_converted * 100.0f, m->starch_gelatinized * 100.0f);
    fd_ui_text(ui, x + 8.0f, cy, 0.85f, (fd_color_t){ 210, 218, 234, 255 }, line);
    cy += 18.0f;

    snprintf(line, sizeof(line), "Taste [0-10] Umami:%.1f Salty:%.1f Sweet:%.1f Sour:%.1f Bitter:%.1f",
             m->umami, m->salty, m->sweet, m->sour, m->bitter);
    fd_ui_text(ui, x + 8.0f, cy, 0.84f, (fd_color_t){ 242, 206, 118, 255 }, line);
    cy += 18.0f;

    snprintf(line, sizeof(line), "Texture [0-10] Crisp:%.1f Tender:%.1f Juicy:%.1f | Emul:%.0f%%",
             m->crispness, m->tenderness, m->juiciness, m->emulsion_stability * 100.0f);
    fd_ui_text(ui, x + 8.0f, cy, 0.84f, (fd_color_t){ 176, 214, 248, 255 }, line);
    cy += 18.0f;

    snprintf(line, sizeof(line), "Nutrition: %.0f kcal (%.0f/100g) P:%.1fg F:%.1fg C:%.1fg RS:%.1fg",
             m->kcal_total, m->kcal_per_100g, m->protein_g, m->fat_g, m->net_carbs_g, m->resistant_starch_g);
    fd_ui_text(ui, x + 8.0f, cy, 0.84f, (fd_color_t){ 198, 238, 192, 255 }, line);
    cy += 18.0f;

    snprintf(line, sizeof(line), "Digestion: eGI %.0f | eGL %.1f | Protein Bioavail %.0f%%",
             m->estimated_gi, m->estimated_gl, m->protein_bioavailability * 100.0f);
    fd_ui_text(ui, x + 8.0f, cy, 0.84f, (fd_color_t){ 198, 238, 192, 255 }, line);
}

static void render_slice_panel(fd_app_t *app, float x, float y, float w, float h) {
    fd_ui_t *ui = &app->ui;
    float bh = ui->touch_mode ? 48.0f : 28.0f;
    fd_ui_rect(ui, x, y, w, h, (fd_color_t){ 22, 26, 36, 255 });
    fd_ui_outline(ui, x, y, w, h, 1.0f, (fd_color_t){ 68, 82, 106, 255 });

    /* Plane toggles XY / XZ / YZ + ColdSpot / HotSpot snaps */
    float bw = (w - 28.0f) / 5.0f;
    if (fd_ui_button(ui, 201, x + 4.0f, y + 4.0f, bw, bh, "XY", app->slice_plane == FD_SLICE_XY)) {
        app->slice_plane = FD_SLICE_XY; refresh_slice(app);
    }
    if (fd_ui_button(ui, 202, x + 8.0f + bw, y + 4.0f, bw, bh, "XZ", app->slice_plane == FD_SLICE_XZ)) {
        app->slice_plane = FD_SLICE_XZ; refresh_slice(app);
    }
    if (fd_ui_button(ui, 203, x + 12.0f + 2.0f * bw, y + 4.0f, bw, bh, "YZ", app->slice_plane == FD_SLICE_YZ)) {
        app->slice_plane = FD_SLICE_YZ; refresh_slice(app);
    }
    if (fd_ui_button(ui, 204, x + 16.0f + 3.0f * bw, y + 4.0f, bw, bh, "ColdSpot", false)) {
        fd_app_snap_cold_spot(app);
    }
    if (fd_ui_button(ui, 205, x + 20.0f + 4.0f * bw, y + 4.0f, bw, bh, "PeakChar", false)) {
        fd_app_snap_hot_spot(app);
    }

    /* Overlay mode buttons */
    float oy = y + bh + 8.0f;
    const char *ov_names[5] = { "Temp", "Moist", "Brown", "Prot/Gel", "Safety" };
    for (int i = 0; i < 5; i++) {
        if (fd_ui_button(ui, 210 + i, x + 4.0f + (float)i * (bw + 4.0f), oy, bw, bh,
                         ov_names[i], (int)app->overlay_mode == i)) {
            app->overlay_mode = (fd_overlay_mode_t)i;
            refresh_slice(app);
        }
    }

    float sy = oy + bh + 6.0f;
    if (fd_ui_slider(ui, 220, x + 6.0f, sy, w - 12.0f, bh, "3D Slice Depth", &app->slice_norm, 0.0f, 1.0f, "%.2f")) {
        refresh_slice(app);
    }

    /* Draw 2D slice quad */
    float img_y = sy + bh + 8.0f;
    float img_h = fmaxf(48.0f, h - (img_y - y) - 10.0f);
    float img_w = fminf(w - 16.0f, img_h * 1.25f);
    float img_x = x + (w - img_w) * 0.5f;
    fd_ui_sprite(ui, FD_TEX_SLICE, img_x, img_y, img_w, img_h, 0.0f, 0.0f, 1.0f, 1.0f,
                 (fd_color_t){ 255, 255, 255, 255 });
    fd_ui_outline(ui, img_x, img_y, img_w, img_h, 1.5f, (fd_color_t){ 140, 158, 185, 255 });
}

static void render_controls_panel(fd_app_t *app, float x, float y, float w, float h) {
    fd_ui_t *ui = &app->ui;
    float bh = ui->touch_mode ? 48.0f : 30.0f;
    fd_ui_rect(ui, x, y, w, h, (fd_color_t){ 24, 29, 39, 255 });
    fd_ui_outline(ui, x, y, w, h, 1.0f, (fd_color_t){ 62, 74, 94, 255 });

    float cy = y + 8.0f;
    /* Recipe selector prev/next */
    float bw3 = (w - 20.0f) / 3.0f;
    if (fd_ui_button(ui, 101, x + 6.0f, cy, bw3, bh, "< Prev Dish", false) && app->db.recipe_count > 0) {
        int next = (app->selected_recipe_idx - 1 + app->db.recipe_count) % app->db.recipe_count;
        fd_app_load_recipe_index(app, next);
    }
    if (fd_ui_button(ui, 102, x + 10.0f + bw3, cy, bw3, bh, "Reset Dish", false) && app->db.recipe_count > 0) {
        fd_app_load_recipe_index(app, app->selected_recipe_idx);
    }
    if (fd_ui_button(ui, 103, x + 14.0f + 2.0f * bw3, cy, bw3, bh, "Next Dish >", false) && app->db.recipe_count > 0) {
        int next = (app->selected_recipe_idx + 1) % app->db.recipe_count;
        fd_app_load_recipe_index(app, next);
    }
    cy += bh + 8.0f;

    if (app->selected_recipe_idx >= 0 && app->selected_recipe_idx < app->db.recipe_count) {
        fd_ui_text(ui, x + 8.0f, cy, 0.92f, (fd_color_t){ 250, 212, 112, 255 },
                   app->db.recipes[app->selected_recipe_idx].name);
        cy += 20.0f;
    }

    /* Vessel sprite from external kitchen_atlas.png */
    int tidx = app->sim.active_tech_idx;
    int tile = (tidx == 3 || tidx == 4 || tidx == 5 || tidx == 6) ? 1
             : ((tidx == 7 || tidx == 8 || tidx == 9) ? 2 : ((tidx == 10) ? 3 : 0));
    float u0 = (float)(tile * 64) / 256.0f, u1 = (float)(tile * 64 + 64) / 256.0f;
    fd_ui_sprite(ui, FD_TEX_KITCHEN, x + 12.0f, cy, 56.0f, 56.0f, u0, 0.0f, u1, 0.5f,
                 (fd_color_t){ 255, 255, 255, 255 });
    if (app->db.technique_count > 0) {
        const fd_technique_t *tc = &app->db.techniques[tidx];
        if (fd_ui_button(ui, 104, x + 76.0f, cy + 6.0f, w - 84.0f, bh, tc->name, false)) {
            int nt = (tidx + 1) % app->db.technique_count;
            fd_sim_set_technique(&app->sim, &app->db, app->db.techniques[nt].id,
                                 app->db.techniques[nt].default_temp_c, app->sim.lid_closed);
            app->pending_cue = FD_CUE_CHOP;
        }
    }
    cy += 62.0f;

    /* Setpoint temperature & time warp sliders */
    float min_t = 4.0f, max_t = 265.0f;
    if (app->db.technique_count > 0) {
        min_t = app->db.techniques[tidx].min_temp_c;
        max_t = app->db.techniques[tidx].max_temp_c;
    }
    fd_ui_slider(ui, 105, x + 6.0f, cy, w - 12.0f, bh, "Vessel Setpoint °C",
                 &app->sim.ambient_temp_c, min_t, max_t, "%.0f°C");
    cy += bh + 8.0f;

    fd_ui_slider(ui, 106, x + 6.0f, cy, w - 12.0f, bh, "Time Warp",
                 &app->time_scale, 1.0f, 120.0f, "%.0fx");
    cy += bh + 8.0f;

    /* Pause / Flip 3D / Lid buttons */
    if (fd_ui_button(ui, 107, x + 6.0f, cy, bw3, bh, app->paused ? "Resume" : "Pause", app->paused)) {
        app->paused = !app->paused;
    }
    if (fd_ui_button(ui, 108, x + 10.0f + bw3, cy, bw3, bh, "Flip / Toss 3D", false)) {
        if (app->sim.grid.nx == app->sim.grid.nz && app->sim.grid.nx > 1) fd_grid_toss_cubes(&app->sim.grid);
        else fd_grid_flip_z(&app->sim.grid);
        refresh_slice(app);
        app->pending_cue = FD_CUE_SEAR_DROP;
    }
    if (fd_ui_button(ui, 109, x + 14.0f + 2.0f * bw3, cy, bw3, bh,
                     app->sim.lid_closed ? "Lid: ON" : "Lid: OFF", app->sim.lid_closed)) {
        app->sim.lid_closed = !app->sim.lid_closed;
    }
    cy += bh + 10.0f;

    if (fd_ui_button(ui, 110, x + 6.0f, cy, w - 12.0f, bh,
                     "Auto-Complete Selected Classic Recipe", false)) {
        if (app->selected_recipe_idx >= 0 && app->selected_recipe_idx < app->db.recipe_count) {
            fd_sim_run_recipe(&app->sim, &app->db, &app->lua, &app->db.recipes[app->selected_recipe_idx], false);
            refresh_slice(app);
            app->pending_cue = FD_CUE_TIMER;
            snprintf(app->status_msg, sizeof(app->status_msg), "Simulated full recipe: %s (Score %.1f)",
                     app->db.recipes[app->selected_recipe_idx].name, app->sim.metrics.overall_score);
        }
    }
}

static void render_propose_tab(fd_app_t *app, float x, float y, float w, float h) {
    fd_ui_t *ui = &app->ui;
    float bh = ui->touch_mode ? 48.0f : 32.0f;
    fd_ui_rect(ui, x, y, w, h, (fd_color_t){ 22, 26, 36, 255 });
    fd_ui_outline(ui, x, y, w, h, 1.0f, (fd_color_t){ 62, 74, 94, 255 });

    float cy = y + 10.0f;
    fd_ui_text(ui, x + 12.0f, cy, 1.0f, (fd_color_t){ 248, 212, 112, 255 },
               "Propose Known Classics (With Chemical Functional Substitution)");
    cy += 26.0f;

    float bw4 = (w - 30.0f) / 4.0f;
    bool is_vegan = (app->constraints.required_diet_flags & FD_DIET_VEGAN) != 0;
    bool is_gf    = (app->constraints.required_diet_flags & FD_DIET_GLUTEN_FREE) != 0;
    bool is_df    = (app->constraints.required_diet_flags & FD_DIET_DAIRY_FREE) != 0;
    bool low_gi   = (app->constraints.max_gi > 0.0f && app->constraints.max_gi <= 55.0f);

    if (fd_ui_button(ui, 301, x + 6.0f, cy, bw4, bh, "Vegan", is_vegan)) {
        app->constraints.required_diet_flags ^= FD_DIET_VEGAN;
    }
    if (fd_ui_button(ui, 302, x + 12.0f + bw4, cy, bw4, bh, "Gluten-Free", is_gf)) {
        app->constraints.required_diet_flags ^= FD_DIET_GLUTEN_FREE;
    }
    if (fd_ui_button(ui, 303, x + 18.0f + 2.0f * bw4, cy, bw4, bh, "Dairy-Free", is_df)) {
        app->constraints.required_diet_flags ^= FD_DIET_DAIRY_FREE;
    }
    if (fd_ui_button(ui, 304, x + 24.0f + 3.0f * bw4, cy, bw4, bh, "Low-GI (<=52)", low_gi)) {
        app->constraints.max_gi = low_gi ? 0.0f : 52.0f;
    }
    cy += bh + 8.0f;

    fd_ui_slider(ui, 305, x + 8.0f, cy, w * 0.55f, bh, "Max Cook Time (s)",
                 &app->constraints.max_time_sec, 180.0f, 9000.0f, "%.0fs");
    if (fd_ui_button(ui, 306, x + w * 0.58f, cy, w * 0.40f, bh, "Match & Verify Classics", true)) {
        app->proposal_count = fd_solver_propose_classics(&app->db, &app->lua, &app->constraints,
                                                         app->proposals, 5);
        app->pending_cue = FD_CUE_TIMER;
        snprintf(app->status_msg, sizeof(app->status_msg), "Proposed %d verified classic recipes.", app->proposal_count);
    }
    cy += bh + 12.0f;

    for (int i = 0; i < app->proposal_count; i++) {
        const fd_proposal_t *p = &app->proposals[i];
        float row_h = bh + 22.0f;
        if (cy + row_h > y + h - 6.0f) break;
        fd_ui_rect(ui, x + 8.0f, cy, w - 16.0f, row_h, (fd_color_t){ 32, 39, 52, 255 });
        char info[220];
        snprintf(info, sizeof(info), "#%d %.42s | Score %.1f | eGI %.0f | %.72s",
                 i + 1, p->recipe.name, p->metrics.overall_score, p->metrics.estimated_gi, p->rationale);
        fd_ui_text(ui, x + 14.0f, cy + 6.0f, 0.86f, (fd_color_t){ 235, 240, 248, 255 }, info);
        if (fd_ui_button(ui, 320 + i, x + w - 150.0f, cy + 4.0f, 136.0f, bh, "Cook Dish", false)) {
            fd_app_load_recipe(app, &p->recipe);
            app->active_tab = FD_TAB_WORKBENCH;
        }
        cy += row_h + 6.0f;
    }
}

static void render_invent_tab(fd_app_t *app, float x, float y, float w, float h) {
    fd_ui_t *ui = &app->ui;
    float bh = ui->touch_mode ? 48.0f : 32.0f;
    fd_ui_rect(ui, x, y, w, h, (fd_color_t){ 22, 26, 36, 255 });
    fd_ui_outline(ui, x, y, w, h, 1.0f, (fd_color_t){ 62, 74, 94, 255 });

    float cy = y + 10.0f;
    fd_ui_text(ui, x + 12.0f, cy, 1.0f, (fd_color_t){ 248, 212, 112, 255 },
               "Invent New Recipe (3D Multi-Fidelity Evolutionary Solver: 8^3 -> Full 3D)");
    cy += 26.0f;

    float hw = (w - 24.0f) * 0.5f;
    if (app->constraints.target_crispness < 0.0f) app->constraints.target_crispness = 6.5f;
    if (app->constraints.target_umami < 0.0f)     app->constraints.target_umami     = 7.0f;

    fd_ui_slider(ui, 401, x + 8.0f, cy, hw, bh, "Target Crispness", &app->constraints.target_crispness, 0.0f, 10.0f, "%.1f");
    fd_ui_slider(ui, 402, x + 16.0f + hw, cy, hw, bh, "Target Umami", &app->constraints.target_umami, 0.0f, 10.0f, "%.1f");
    cy += bh + 8.0f;

    fd_ui_slider(ui, 403, x + 8.0f, cy, hw, bh, "Min Protein (g)", &app->constraints.min_protein_g, 0.0f, 55.0f, "%.0fg");
    if (fd_ui_button(ui, 404, x + 16.0f + hw, cy, hw * 0.48f, bh,
                     app->constraints.require_emulsion ? "Emulsion: ON" : "Emulsion: OFF",
                     app->constraints.require_emulsion)) {
        app->constraints.require_emulsion = !app->constraints.require_emulsion;
    }
    if (fd_ui_button(ui, 405, x + 16.0f + hw * 1.50f, cy, hw * 0.50f, bh, "Invent Recipe!", true)) {
        app->constraints.rng_seed += 1337u;
        fd_inventor_start(&app->inventor, &app->db, &app->constraints);
        snprintf(app->status_msg, sizeof(app->status_msg), "Running 3D evolutionary search...");
    }
    cy += bh + 12.0f;

    /* Search progress bar */
    float prog = fd_inventor_progress(&app->inventor);
    fd_ui_rect(ui, x + 8.0f, cy, w - 16.0f, 18.0f, (fd_color_t){ 34, 40, 54, 255 });
    fd_ui_rect(ui, x + 8.0f, cy, (w - 16.0f) * prog, 18.0f, (fd_color_t){ 64, 198, 126, 255 });
    cy += 28.0f;

    if (app->inventor.done || app->inventor.evals_done > 0) {
        const fd_proposal_t *b = &app->inventor.best;
        char line[192];
        snprintf(line, sizeof(line), "%s  [Score %.1f/100 | Safe: %s | %.0f kcal]",
                 b->recipe.name, b->metrics.overall_score,
                 b->metrics.is_pasteurized_safe ? "YES" : "NO", b->metrics.kcal_total);
        fd_ui_text(ui, x + 12.0f, cy, 0.95f, (fd_color_t){ 112, 235, 162, 255 }, line);
        cy += 22.0f;

        fd_ui_text(ui, x + 12.0f, cy, 0.85f, (fd_color_t){ 198, 212, 232, 255 }, b->rationale);
        cy += 20.0f;

        for (int i = 0; i < b->recipe.item_count && cy < y + h - 70.0f; i++) {
            int idx = fd_db_find_ingredient(&app->db, b->recipe.items[i].ingredient_id);
            const char *nm = (idx >= 0) ? app->db.ingredients[idx].name : b->recipe.items[i].ingredient_id;
            snprintf(line, sizeof(line), "  - %.1f g  %s", b->recipe.items[i].mass_g, nm);
            fd_ui_text(ui, x + 12.0f, cy, 0.85f, (fd_color_t){ 232, 238, 246, 255 }, line);
            cy += 17.0f;
        }
        for (int s = 0; s < b->recipe.step_count && cy < y + h - 45.0f; s++) {
            snprintf(line, sizeof(line), "  Step %d: %s @ %.0f°C for %.0fs %s",
                     s + 1, b->recipe.steps[s].technique_id, b->recipe.steps[s].temp_c,
                     b->recipe.steps[s].duration_sec, b->recipe.steps[s].flip_midway ? "(Flip midway)" : "");
            fd_ui_text(ui, x + 12.0f, cy, 0.85f, (fd_color_t){ 246, 206, 118, 255 }, line);
            cy += 18.0f;
        }
        if (app->inventor.done &&
            fd_ui_button(ui, 410, x + 12.0f, y + h - bh - 10.0f, 240.0f, bh, "Load Invented Dish in Workbench", true)) {
            fd_app_load_recipe(app, &b->recipe);
            app->active_tab = FD_TAB_WORKBENCH;
        }
    }
}

static void render_mods_tab(fd_app_t *app, float x, float y, float w, float h) {
    fd_ui_t *ui = &app->ui;
    float bh = ui->touch_mode ? 48.0f : 34.0f;
    fd_ui_rect(ui, x, y, w, h, (fd_color_t){ 22, 26, 36, 255 });
    fd_ui_outline(ui, x, y, w, h, 1.0f, (fd_color_t){ 62, 74, 94, 255 });

    float cy = y + 12.0f;
    fd_ui_text(ui, x + 12.0f, cy, 1.0f, (fd_color_t){ 248, 212, 112, 255 },
               "External Assets & Sandboxed Lua 5.4 Mod Lab");
    cy += 26.0f;

    char summary[220];
    snprintf(summary, sizeof(summary),
             "Assets Dir: %.64s | Ingredients: %d | Techniques: %d | Recipes: %d | Active Mods: %d",
             app->vfs.base_dir, app->db.ingredient_count, app->db.technique_count,
             app->db.recipe_count, app->lua.mod_count);
    fd_ui_text(ui, x + 12.0f, cy, 0.88f, (fd_color_t){ 215, 228, 245, 255 }, summary);
    cy += 28.0f;

    if (fd_ui_button(ui, 501, x + 12.0f, cy, 260.0f, bh, "Hot-Reload ./assets/scripts/ (F5)", true)) {
        if (fd_lua_reload_all(&app->lua)) {
            app->pending_cue = FD_CUE_TIMER;
            snprintf(app->status_msg, sizeof(app->status_msg),
                     "Reloaded Lua database (%d ingredients, %d recipes).",
                     app->db.ingredient_count, app->db.recipe_count);
        }
    }

    if (fd_ui_button(ui, 502, x + 284.0f, cy, 310.0f, bh,
                     app->mod_boost_enabled ? "Disable Truffle Mod Override" : "Inject Live Lua Truffle Mod Override",
                     app->mod_boost_enabled)) {
        app->mod_boost_enabled = !app->mod_boost_enabled;
        if (app->mod_boost_enabled) {
            const char *mod_src =
                "fatduck.add_ingredient({ id='black_garlic', name='Aged Black Garlic (Mod)', category='umami',"
                " water=0.52, protein=0.08, sugar=0.24, ph=4.3, glutamate_mg_100g=640, diet={'vegan','gluten_free'}, roles={'umami','aromatic'} })\n"
                "fatduck.add_ingredient({ id='perigord_truffle', name='Perigord Black Truffle (Live Mod)',"
                " category='fungi', water=0.75, protein=0.09, fat=0.01, fiber=0.08, ph=6.3,"
                " glutamate_mg_100g=320, nucleotides_mg_100g=180, aroma_potency=1.0,"
                " diet={'vegan','gluten_free'}, roles={'umami','aromatic'} })\n"
                "fatduck.register_mod({ name='Truffle Lab', on_score_dish=function(m) return 3.0 end })\n";
            fd_vfs_put(&app->vfs, "scripts/mods/example_mod.lua", mod_src, strlen(mod_src), true);
        } else {
            fd_vfs_clear_override(&app->vfs, "scripts/mods/example_mod.lua");
        }
        fd_lua_reload_all(&app->lua);
        snprintf(app->status_msg, sizeof(app->status_msg),
                 app->mod_boost_enabled ? "Applied live VFS Lua mod override!" : "Restored disk example_mod.lua.");
    }
    cy += bh + 18.0f;

    fd_ui_text(ui, x + 12.0f, cy, 0.86f, (fd_color_t){ 182, 198, 220, 255 },
               "Modding tip: Edit ./assets/scripts/*.lua on Desktop or use localStorage overrides on Web.");
}

void fd_app_update_and_render(fd_app_t *app, const fd_input_t *in, float frame_dt_sec) {
    if (!app || !in) return;
    app->pending_cue = FD_CUE_NONE;

    /* Auto-switch to touch mode on touch event or narrow mobile viewport */
    if (!app->touch_manual_override) {
        if (in->is_touch_event || in->width < 800.0f) app->touch_mode = true;
    }

    /* Handle hotkeys */
    if (in->key_space) app->paused = !app->paused;
    if (in->key_flip && app->sim.initialized) {
        fd_grid_flip_z(&app->sim.grid);
        refresh_slice(app);
        app->pending_cue = FD_CUE_SEAR_DROP;
    }
    if (in->key_plane) {
        app->slice_plane = (fd_slice_plane_t)(((int)app->slice_plane + 1) % 3);
        refresh_slice(app);
    }
    if (in->key_reload) {
        fd_lua_reload_all(&app->lua);
    }
    if (in->speed_key >= 1 && in->speed_key <= 5) {
        static const float speeds[5] = { 1.0f, 5.0f, 15.0f, 45.0f, 120.0f };
        app->time_scale = speeds[in->speed_key - 1];
    }

    /* Step frame-sliced 3D evolutionary recipe inventor if active */
    if (app->inventor.active && !app->inventor.done) {
        if (fd_inventor_step(&app->inventor, &app->db, &app->lua, 12)) {
            app->pending_cue = FD_CUE_TIMER;
            snprintf(app->status_msg, sizeof(app->status_msg),
                     "Invented '%s' (Score %.1f/100)!",
                     app->inventor.best.recipe.name, app->inventor.best.metrics.overall_score);
        }
    }

    /* Advance live 3D simulation */
    if (!app->paused && app->sim.initialized && frame_dt_sec > 0.0f && app->active_tab == FD_TAB_WORKBENCH) {
        float sim_dt = clampf(frame_dt_sec, 0.0f, 0.1f) * app->time_scale;
        fd_sim_step(&app->sim, &app->db, &app->lua, sim_dt);
        refresh_slice(app);
    }

    /* Render UI */
    fd_ui_begin(&app->ui, in, app->touch_mode);
    float W = fmaxf(320.0f, in->width);
    float H = fmaxf(320.0f, in->height);
    fd_ui_rect(&app->ui, 0.0f, 0.0f, W, H, (fd_color_t){ 14, 17, 24, 255 });

    /* Top header bar */
    float top_h = app->touch_mode ? 52.0f : 42.0f;
    fd_ui_rect(&app->ui, 0.0f, 0.0f, W, top_h, (fd_color_t){ 24, 30, 42, 255 });
    fd_ui_text(&app->ui, 12.0f, (top_h - 16.0f) * 0.5f, 1.05f, (fd_color_t){ 248, 198, 76, 255 },
               "FatDuck 3D Culinary Lab");

    if (fd_ui_button(&app->ui, 10, W - 126.0f, 4.0f, 118.0f, top_h - 8.0f,
                     app->touch_mode ? "Mode: Touch" : "Mode: Desktop", app->touch_mode)) {
        app->touch_mode = !app->touch_mode;
        app->touch_manual_override = true;
    }

    const char *tab_labels[4] = { "Workbench", "Propose", "Invent", "Mods" };
    float body_y = top_h + 6.0f;
    float status_h = 24.0f;
    float body_h = H - body_y - status_h - 6.0f;

    if (app->touch_mode) {
        /* Mobile Touch layout: bottom thumb bar (>=48px) + single focused stack */
        float nav_h = 54.0f;
        body_h -= nav_h;
        float tw = (W - 20.0f) / 4.0f;
        for (int t = 0; t < 4; t++) {
            if (fd_ui_button(&app->ui, 20 + t, 4.0f + (float)t * (tw + 4.0f), H - nav_h - status_h,
                             tw, nav_h - 4.0f, tab_labels[t], (int)app->active_tab == t)) {
                app->active_tab = (fd_tab_t)t;
            }
        }
    } else {
        /* Desktop top navigation tabs */
        float tw = 112.0f;
        for (int t = 0; t < 4; t++) {
            if (fd_ui_button(&app->ui, 20 + t, 255.0f + (float)t * (tw + 6.0f), 5.0f,
                             tw, top_h - 10.0f, tab_labels[t], (int)app->active_tab == t)) {
                app->active_tab = (fd_tab_t)t;
            }
        }
    }

    if (app->active_tab == FD_TAB_WORKBENCH) {
        if (app->touch_mode) {
            float sub_w = (W - 20.0f) / 3.0f;
            const char *subs[3] = { "1. Station", "2. 3D Slice", "3. Metrics" };
            for (int s = 0; s < 3; s++) {
                if (fd_ui_button(&app->ui, 50 + s, 4.0f + (float)s * (sub_w + 6.0f), body_y,
                                 sub_w, 48.0f, subs[s], app->mobile_subtab == s)) {
                    app->mobile_subtab = s;
                }
            }
            float cy = body_y + 54.0f, ch = body_h - 54.0f;
            if (app->mobile_subtab == 0)      render_controls_panel(app, 6.0f, cy, W - 12.0f, ch);
            else if (app->mobile_subtab == 1) render_slice_panel(app, 6.0f, cy, W - 12.0f, ch);
            else {
                draw_metrics_card(app, 6.0f, cy, W - 12.0f, 152.0f);
                draw_telemetry_plot(app, 6.0f, cy + 158.0f, W - 12.0f, fmaxf(80.0f, ch - 158.0f));
            }
        } else {
            float col_l = fminf(340.0f, W * 0.30f);
            float col_m = (W - col_l - 24.0f) * 0.48f;
            float col_r = W - col_l - col_m - 24.0f;
            render_controls_panel(app, 6.0f, body_y, col_l, body_h);
            render_slice_panel(app, 12.0f + col_l, body_y, col_m, body_h);
            draw_metrics_card(app, 18.0f + col_l + col_m, body_y, col_r, 154.0f);
            draw_telemetry_plot(app, 18.0f + col_l + col_m, body_y + 160.0f, col_r, body_h - 160.0f);
        }
    } else if (app->active_tab == FD_TAB_PROPOSE) {
        render_propose_tab(app, 6.0f, body_y, W - 12.0f, body_h);
    } else if (app->active_tab == FD_TAB_INVENT) {
        render_invent_tab(app, 6.0f, body_y, W - 12.0f, body_h);
    } else {
        render_mods_tab(app, 6.0f, body_y, W - 12.0f, body_h);
    }

    /* Footer status bar */
    fd_ui_rect(&app->ui, 0.0f, H - status_h, W, status_h, (fd_color_t){ 20, 24, 34, 255 });
    fd_ui_text(&app->ui, 10.0f, H - status_h + 4.0f, 0.84f, (fd_color_t){ 192, 208, 232, 255 }, app->status_msg);
}
