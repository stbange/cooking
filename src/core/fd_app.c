#include "fd_app.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define FD_ONLINE_DOCS_URL "https://github.com/stbange/cooking/blob/main/docs/HELP.md"

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static const char *T(const fd_app_t *app, const char *key) {
    return fd_tr(&app->i18n, key);
}

static void set_status(fd_app_t *app, const char *key, const char *detail) {
    const char *base = T(app, key);
    if (detail && detail[0] != '\0') {
        snprintf(app->status_msg, sizeof(app->status_msg), "%s: %s", base, detail);
    } else {
        snprintf(app->status_msg, sizeof(app->status_msg), "%s", base);
    }
}

static void refresh_slice(fd_app_t *app) {
    const fd_sim_state_t *sv = fd_app_vessel_const(app);
    if (!sv->initialized) return;
    const fd_grid_t *g = &sv->grid;
    int max_idx = (app->slice_plane == FD_SLICE_XY) ? (g->nz - 1)
                : ((app->slice_plane == FD_SLICE_XZ) ? (g->ny - 1) : (g->nx - 1));
    int sidx = (int)roundf(clampf(app->slice_norm, 0.0f, 1.0f) * (float)(max_idx > 0 ? max_idx : 0));
    fd_grid_extract_slice(g, app->slice_plane, sidx, app->overlay_mode,
                          app->slice_rgba, &app->slice_w, &app->slice_h);
    app->slice_dirty = true;
}

bool fd_app_load_recipe(fd_app_t *app, const fd_recipe_t *r) {
    if (!app || !r || r->item_count <= 0) return false;
    if (!fd_kitchen_init_recipe(&app->kitchen, &app->db, r, false)) {
        return false;
    }
    app->slice_plane = FD_SLICE_XZ;
    app->slice_norm  = 0.5f;
    refresh_slice(app);
    app->pending_cue = FD_CUE_SEAR_DROP;
    set_status(app, "status_loaded", r->name);
    return true;
}

bool fd_app_load_recipe_index(fd_app_t *app, int recipe_idx) {
    if (!app || recipe_idx < 0 || recipe_idx >= app->db.recipe_count) return false;
    app->selected_recipe_idx = recipe_idx;
    return fd_app_load_recipe(app, &app->db.recipes[recipe_idx]);
}

bool fd_app_select_vessel(fd_app_t *app, int vessel_idx) {
    if (!app || vessel_idx < 0 || vessel_idx >= app->kitchen.vessel_count) return false;
    app->kitchen.active_vessel = vessel_idx;
    refresh_slice(app);
    set_status(app, "status_loaded", app->kitchen.vessels[vessel_idx].vessel_name);
    return true;
}

bool fd_app_step_recipe_next(fd_app_t *app) {
    if (!app || !app->kitchen.has_recipe) return false;
    if (app->kitchen.current_step >= app->kitchen.recipe.step_count) return false;
    int s_idx = app->kitchen.current_step;
    char instr[200];
    fd_recipe_describe_step(&app->kitchen.recipe, &app->db, s_idx, instr, sizeof(instr), NULL, 0);
    if (!fd_kitchen_exec_next_step(&app->kitchen, &app->db, &app->lua)) return false;
    refresh_slice(app);
    app->pending_cue = FD_CUE_TIMER;
    set_status(app, "status_step_done", instr);
    return true;
}

bool fd_app_trigger_action(fd_app_t *app, fd_step_action_t action) {
    if (!app) return false;
    fd_sim_state_t *sv = fd_app_vessel(app);
    if (!sv->initialized || sv->consumed) return false;

    if (action == FD_ACT_COMBINE) {
        int cur = app->kitchen.active_vessel;
        int dst = -1;
        for (int i = 1; i < app->kitchen.vessel_count; i++) {
            int cand = (cur + i) % app->kitchen.vessel_count;
            if (!app->kitchen.vessels[cand].consumed) { dst = cand; break; }
        }
        if (dst < 0 || dst == cur) return false;
        if (!fd_sim_combine_vessels(&app->kitchen.vessels[dst], sv, &app->db, &app->lua, true, false)) {
            return false;
        }
        app->kitchen.active_vessel = dst;
        refresh_slice(app);
        app->pending_cue = FD_CUE_SEAR_DROP;
        set_status(app, "status_combined", app->kitchen.vessels[dst].vessel_name);
        return true;
    }

    bool ok = fd_sim_apply_action(sv, &app->db, &app->lua, action,
                                  (action == FD_ACT_FOLD_STIR) ? 0.25f : 0.85f,
                                  0.0f, 0.0f, 0.0f, sv->shape, false,
                                  sv->ambient_temp_c, 45.0f, false);
    if (ok) {
        refresh_slice(app);
        app->pending_cue = FD_CUE_CHOP;
        set_status(app, "status_action", sv->vessel_name);
    }
    return ok;
}

bool fd_app_set_language(fd_app_t *app, int lang_idx) {
    if (!app) return false;
    bool ok = fd_i18n_set_lang(&app->i18n, &app->lua, lang_idx);
    if (app->ui.initialized) {
        fd_ui_rebuild_fonts(&app->ui, &app->vfs, &app->i18n, app->ui.baked_dpi_scale);
    }
    app->i18n.font_dirty = false;
    app->lang_changed    = true;
    set_status(app, "status_ready", fd_i18n_lang_info(app->i18n.active_lang)->english_name);
    return ok;
}

void fd_app_start_tutorial(fd_app_t *app) {
    if (!app) return;
    app->show_welcome     = false;
    app->help_open        = false;
    app->recipe_card_open = false;
    app->tutorial_open    = true;
    app->tutorial_step    = 0;
}

void fd_app_tutorial_try_step(fd_app_t *app, int step) {
    if (!app) return;
    if (step == 0) {
        app->active_tab    = FD_TAB_WORKBENCH;
        app->mobile_subtab = 0;
        int idx = fd_db_find_recipe(&app->db, "pasta_carbonara");
        fd_app_load_recipe_index(app, idx >= 0 ? idx : 0);
    } else if (step == 1) {
        app->active_tab    = FD_TAB_WORKBENCH;
        app->mobile_subtab = 1;
        app->overlay_mode  = FD_OVERLAY_TEMP;
        app->slice_plane   = FD_SLICE_XZ;
        fd_app_snap_cold_spot(app);
    } else if (step == 2) {
        app->active_tab    = FD_TAB_WORKBENCH;
        app->mobile_subtab = 0;
        if (!fd_app_step_recipe_next(app)) {
            fd_sim_state_t *sv = fd_app_vessel(app);
            if (sv->initialized) {
                fd_sim_step(sv, &app->db, &app->lua, 25.0f);
                fd_grid_flip_z(&sv->grid);
                refresh_slice(app);
            }
        }
    } else if (step == 3) {
        app->active_tab = FD_TAB_PROPOSE;
        app->proposal_count = fd_solver_propose_classics(&app->db, &app->lua, &app->constraints,
                                                         app->proposals, 5);
        set_status(app, "status_proposed", NULL);
    } else if (step == 4) {
        app->active_tab = FD_TAB_INVENT;
        app->constraints.rng_seed += 1337u;
        fd_inventor_start(&app->inventor, &app->db, &app->constraints);
        set_status(app, "status_inventing", NULL);
    } else {
        app->active_tab = FD_TAB_MODS;
    }
}

void fd_app_open_help(fd_app_t *app, int tab) {
    if (!app) return;
    app->tutorial_open    = false;
    app->recipe_card_open = false;
    app->help_open        = true;
    if (tab >= 0 && tab < FD_HELP_TABS) app->help_tab = tab;
}

bool fd_app_init_with_lang(fd_app_t *app, const char *explicit_assets_dir,
                           const char *exe_base_path, const char *lang_code) {
    if (!app) return false;
    memset(app, 0, sizeof(*app));
    fd_vfs_init(&app->vfs, explicit_assets_dir, exe_base_path);
    if (!fd_lua_init(&app->lua, &app->vfs, &app->db)) return false;
    if (!fd_lua_exec_file(&app->lua, "scripts/init.lua", 2000000)) {
        snprintf(app->status_msg, sizeof(app->status_msg), "Lua error: %.120s", app->lua.last_error);
        return false;
    }

    fd_i18n_init(&app->i18n, &app->lua, lang_code ? lang_code : "en");
    if (!fd_ui_init(&app->ui, &app->vfs, &app->i18n)) {
        fd_app_free(app);
        return false;
    }

    app->time_scale       = 5.0f;
    app->paused           = false;
    app->show_welcome     = true;
    app->pending_lang_idx = -1;
    app->slice_plane      = FD_SLICE_XZ;
    app->slice_norm       = 0.5f;
    app->overlay_mode     = FD_OVERLAY_TEMP;
    fd_constraints_init_default(&app->constraints);

    if (app->db.recipe_count > 0) {
        fd_app_load_recipe_index(app, 0);
    }
    set_status(app, "status_ready", NULL);
    return true;
}

bool fd_app_init(fd_app_t *app, const char *explicit_assets_dir, const char *exe_base_path) {
    return fd_app_init_with_lang(app, explicit_assets_dir, exe_base_path, "en");
}

void fd_app_free(fd_app_t *app) {
    if (!app) return;
    fd_ui_free(&app->ui);
    fd_i18n_free(&app->i18n);
    fd_kitchen_free(&app->kitchen);
    fd_lua_free(&app->lua);
    fd_vfs_free(&app->vfs);
}

void fd_app_snap_cold_spot(fd_app_t *app) {
    if (!app) return;
    const fd_sim_state_t *sv = fd_app_vessel_const(app);
    if (!sv->initialized) return;
    const fd_grid_t *g = &sv->grid;
    const fd_metrics_t *m = &sv->metrics;
    if (app->slice_plane == FD_SLICE_XY)      app->slice_norm = (g->nz > 1) ? ((float)m->cold_z / (float)(g->nz - 1)) : 0.5f;
    else if (app->slice_plane == FD_SLICE_XZ) app->slice_norm = (g->ny > 1) ? ((float)m->cold_y / (float)(g->ny - 1)) : 0.5f;
    else                                      app->slice_norm = (g->nx > 1) ? ((float)m->cold_x / (float)(g->nx - 1)) : 0.5f;
    refresh_slice(app);
    char detail[64];
    snprintf(detail, sizeof(detail), "%.1f°C (%.1fD)", m->cold_spot_temp_c, m->cold_spot_lethality_log10);
    set_status(app, "status_cold_snapped", detail);
}

void fd_app_snap_hot_spot(fd_app_t *app) {
    if (!app) return;
    const fd_sim_state_t *sv = fd_app_vessel_const(app);
    if (!sv->initialized) return;
    const fd_grid_t *g = &sv->grid;
    const fd_metrics_t *m = &sv->metrics;
    if (app->slice_plane == FD_SLICE_XY)      app->slice_norm = (g->nz > 1) ? ((float)m->hot_z / (float)(g->nz - 1)) : 0.5f;
    else if (app->slice_plane == FD_SLICE_XZ) app->slice_norm = (g->ny > 1) ? ((float)m->hot_y / (float)(g->ny - 1)) : 0.5f;
    else                                      app->slice_norm = (g->nx > 1) ? ((float)m->hot_x / (float)(g->nx - 1)) : 0.5f;
    refresh_slice(app);
    char detail[64];
    snprintf(detail, sizeof(detail), "%.1f°C (%.0f%%)", m->surface_max_temp_c, m->max_char * 100.0f);
    set_status(app, "status_hot_snapped", detail);
}

static void draw_section_title(fd_app_t *app, const char *title) {
    struct nk_context *ctx = &app->ui.nk;
    if (app->ui.font_title) nk_style_push_font(ctx, &app->ui.uf_title);
    nk_layout_row_dynamic(ctx, 24.0f, 1);
    nk_label_colored(ctx, title, NK_TEXT_LEFT, nk_rgba(248, 198, 86, 255));
    if (app->ui.font_title) nk_style_pop_font(ctx);
}

static void render_controls_panel(fd_app_t *app, float x, float y, float w, float h) {
    struct nk_context *ctx = &app->ui.nk;
    float bh = app->touch_mode ? 48.0f : 30.0f;
    fd_sim_state_t *sv = fd_app_vessel(app);

    if (nk_begin(ctx, "fd_controls", nk_rect(x, y, w, h), NK_WINDOW_BORDER)) {
        draw_section_title(app, T(app, "sec_recipe"));

        /* Recipe selector dropdown */
        const char *cur_name = app->kitchen.has_recipe ? app->kitchen.recipe.name
                             : ((app->selected_recipe_idx >= 0 && app->selected_recipe_idx < app->db.recipe_count)
                                ? app->db.recipes[app->selected_recipe_idx].name : "---");
        nk_layout_row_dynamic(ctx, bh, 1);
        if (nk_combo_begin_label(ctx, cur_name, nk_vec2(w - 20.0f, 260.0f))) {
            nk_layout_row_dynamic(ctx, app->touch_mode ? 40.0f : 26.0f, 1);
            for (int i = 0; i < app->db.recipe_count; i++) {
                if (nk_combo_item_label(ctx, app->db.recipes[i].name, NK_TEXT_LEFT)) {
                    fd_app_load_recipe_index(app, i);
                    sv = fd_app_vessel(app);
                }
            }
            nk_combo_end(ctx);
        }
        fd_ui_tooltip(ctx, T(app, "tip_recipe"), app->touch_mode);

        nk_layout_row_dynamic(ctx, bh, 4);
        if (fd_ui_button_styled(ctx, T(app, "btn_prev"), false) && app->db.recipe_count > 0) {
            fd_app_load_recipe_index(app, (app->selected_recipe_idx - 1 + app->db.recipe_count) % app->db.recipe_count);
            sv = fd_app_vessel(app);
        }
        if (fd_ui_button_styled(ctx, T(app, "btn_reset"), false) && app->kitchen.has_recipe) {
            fd_recipe_t copy_r = app->kitchen.recipe;
            fd_app_load_recipe(app, &copy_r);
            sv = fd_app_vessel(app);
        }
        if (fd_ui_button_styled(ctx, T(app, "btn_next"), false) && app->db.recipe_count > 0) {
            fd_app_load_recipe_index(app, (app->selected_recipe_idx + 1) % app->db.recipe_count);
            sv = fd_app_vessel(app);
        }
        if (fd_ui_button_pill(ctx, T(app, "btn_recipe_card"), app->recipe_card_open, nk_rgba(235, 166, 52, 255))) {
            app->recipe_card_open = !app->recipe_card_open;
            if (app->recipe_card_open) { app->help_open = false; app->tutorial_open = false; }
        }

        /* Multi-Vessel Kitchen Station Switcher */
        draw_section_title(app, T(app, "sec_vessels"));
        if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
        int vc = app->kitchen.vessel_count > 0 ? app->kitchen.vessel_count : 1;
        int cols = (vc <= 2) ? vc : (app->touch_mode ? 2 : 2);
        nk_layout_row_dynamic(ctx, app->touch_mode ? 42.0f : 27.0f, cols);
        for (int v = 0; v < app->kitchen.vessel_count; v++) {
            const fd_sim_state_t *vv = &app->kitchen.vessels[v];
            char vlbl[64];
            if (vv->consumed) {
                snprintf(vlbl, sizeof(vlbl), "%d. %.16s [->]", v + 1, vv->vessel_name);
            } else if (vv->initialized) {
                snprintf(vlbl, sizeof(vlbl), "%d. %.15s %.0f°C", v + 1, vv->vessel_name, vv->metrics.core_temp_c);
            } else {
                snprintf(vlbl, sizeof(vlbl), "%d. %.18s", v + 1, vv->vessel_name);
            }
            struct nk_color acc = vv->consumed ? nk_rgba(92, 108, 134, 255)
                                : (vv->burner_on ? nk_rgba(242, 146, 62, 255) : nk_rgba(77, 163, 245, 255));
            if (fd_ui_button_pill(ctx, vlbl, app->kitchen.active_vessel == v, acc)) {
                fd_app_select_vessel(app, v);
                sv = fd_app_vessel(app);
            }
        }
        if (app->ui.font_small) nk_style_pop_font(ctx);

        draw_section_title(app, T(app, "sec_station"));

        /* Vessel icon + proportional Technique selector dropdown */
        int tidx = (sv->active_tech_idx >= 0 && sv->active_tech_idx < app->db.technique_count) ? sv->active_tech_idx : 0;
        int tile = (sv->vessel_kind == FD_VESSEL_POT) ? 1
                 : ((sv->vessel_kind == FD_VESSEL_OVEN) ? 2
                 : ((tidx == 10) ? 3 : 0));
        float icon_w = app->touch_mode ? 50.0f : 42.0f;
        nk_layout_row_begin(ctx, NK_STATIC, icon_w, 2);
        nk_layout_row_push(ctx, icon_w);
        struct nk_image vessel_img = nk_subimage_id((int)FD_TEX_KITCHEN, 512, 256,
                                                    nk_rect((float)(tile * 128), 0.0f, 128.0f, 128.0f));
        nk_image(ctx, vessel_img);

        float right_w = fmaxf(120.0f, w - icon_w - 46.0f);
        nk_layout_row_push(ctx, right_w);
        if (nk_group_begin(ctx, "fd_tech_col", NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, app->touch_mode ? 40.0f : 30.0f, 1);
            const char *tname = (app->db.technique_count > 0) ? app->db.techniques[tidx].name : "---";
            if (nk_combo_begin_label(ctx, tname, nk_vec2(w - 42.0f, 260.0f))) {
                nk_layout_row_dynamic(ctx, app->touch_mode ? 40.0f : 26.0f, 1);
                for (int i = 0; i < app->db.technique_count; i++) {
                    if (nk_combo_item_label(ctx, app->db.techniques[i].name, NK_TEXT_LEFT)) {
                        fd_sim_set_technique(sv, &app->db, app->db.techniques[i].id,
                                             app->db.techniques[i].default_temp_c, sv->lid_closed);
                        app->pending_cue = FD_CUE_CHOP;
                    }
                }
                nk_combo_end(ctx);
            }
            fd_ui_tooltip(ctx, T(app, "tip_technique"), app->touch_mode);
            nk_group_end(ctx);
        }
        nk_layout_row_end(ctx);

        /* Temperature setpoint slider + Burner on/off toggle */
        float min_t = 4.0f, max_t = 265.0f;
        if (app->db.technique_count > 0) {
            min_t = app->db.techniques[tidx].min_temp_c;
            max_t = app->db.techniques[tidx].max_temp_c;
        }
        char tbuf[48];
        snprintf(tbuf, sizeof(tbuf), "%.0f°C (%s)", sv->ambient_temp_c, sv->burner_on ? "ON" : "OFF");
        nk_layout_row_begin(ctx, NK_DYNAMIC, 20.0f, 2);
        nk_layout_row_push(ctx, 0.64f);
        nk_label(ctx, T(app, "lbl_temp_setpoint"), NK_TEXT_LEFT);
        nk_layout_row_push(ctx, 0.36f);
        nk_label_colored(ctx, tbuf, NK_TEXT_RIGHT, nk_rgba(248, 198, 86, 255));
        nk_layout_row_end(ctx);

        nk_layout_row_dynamic(ctx, app->touch_mode ? 36.0f : 22.0f, 1);
        nk_slider_float(ctx, min_t, &sv->ambient_temp_c, max_t, 1.0f);
        fd_ui_tooltip(ctx, T(app, "tip_temp"), app->touch_mode);

        /* Time speed preset pills */
        static const float speeds[5] = { 1.0f, 5.0f, 15.0f, 45.0f, 120.0f };
        static const char *slbls[5]  = { "1x", "5x", "15x", "45x", "120x" };
        nk_layout_row_dynamic(ctx, app->touch_mode ? 42.0f : 26.0f, 5);
        for (int s = 0; s < 5; s++) {
            bool act = fabsf(app->time_scale - speeds[s]) < 0.5f;
            if (fd_ui_button_styled(ctx, slbls[s], act)) app->time_scale = speeds[s];
        }

        /* Thermal & Lid row */
        nk_layout_row_dynamic(ctx, bh, 4);
        if (fd_ui_button_styled(ctx, app->paused ? T(app, "btn_resume") : T(app, "btn_pause"), app->paused)) {
            app->paused = !app->paused;
        }
        if (fd_ui_button_pill(ctx, sv->burner_on ? T(app, "btn_burner_on") : T(app, "btn_burner_off"),
                              sv->burner_on, nk_rgba(242, 134, 62, 255))) {
            sv->burner_on = !sv->burner_on;
        }
        if (fd_ui_button_styled(ctx, T(app, "btn_flip"), false) && sv->initialized) {
            if (sv->grid.nx == sv->grid.nz && sv->grid.nx > 1) fd_grid_toss_cubes(&sv->grid);
            else fd_grid_flip_z(&sv->grid);
            refresh_slice(app);
            app->pending_cue = FD_CUE_SEAR_DROP;
        }
        if (fd_ui_button_styled(ctx, sv->lid_closed ? T(app, "btn_lid_on") : T(app, "btn_lid_off"),
                                sv->lid_closed)) {
            sv->lid_closed = !sv->lid_closed;
        }

        /* Non-Thermal Mechanical & Movement Action Bar */
        if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
        nk_layout_row_dynamic(ctx, 18.0f, 1);
        nk_label_colored(ctx, T(app, "sec_actions"), NK_TEXT_LEFT, nk_rgba(166, 190, 224, 255));
        nk_layout_row_dynamic(ctx, app->touch_mode ? 42.0f : 27.0f, 3);
        if (fd_ui_button_styled(ctx, T(app, "btn_act_cut"), false)) {
            fd_app_trigger_action(app, FD_ACT_CUT_PREP);
            sv = fd_app_vessel(app);
        }
        if (fd_ui_button_styled(ctx, T(app, "btn_act_whisk"), false)) {
            fd_app_trigger_action(app, FD_ACT_WHISK);
        }
        if (fd_ui_button_styled(ctx, T(app, "btn_act_knead"), false)) {
            bool is_dough = (sv->dry_starch_g > 8.0f && sv->yeast_viability > 0.01f);
            fd_app_trigger_action(app, is_dough ? FD_ACT_KNEAD : FD_ACT_FOLD_STIR);
        }
        nk_layout_row_dynamic(ctx, app->touch_mode ? 42.0f : 27.0f, 3);
        if (fd_ui_button_styled(ctx, T(app, "btn_act_deglaze"), false)) {
            fd_app_trigger_action(app, FD_ACT_DEGLAZE);
        }
        if (fd_ui_button_styled(ctx, T(app, "btn_act_rest"), false)) {
            fd_app_trigger_action(app, FD_ACT_REST);
        }
        if (fd_ui_button_pill(ctx, T(app, "btn_act_combine"), app->kitchen.vessel_count > 1, nk_rgba(52, 199, 133, 255))) {
            fd_app_trigger_action(app, FD_ACT_COMBINE);
            sv = fd_app_vessel(app);
        }
        if (app->ui.font_small) nk_style_pop_font(ctx);

        /* Step-by-Step Executor & Simulate Full Recipe CTAs */
        int cur_s = app->kitchen.current_step;
        int tot_s = app->kitchen.recipe.step_count;
        char step_btn[64];
        snprintf(step_btn, sizeof(step_btn), "%s (%d/%d)", T(app, "btn_step_next"),
                 cur_s < tot_s ? cur_s + 1 : tot_s, tot_s);
        nk_layout_row_dynamic(ctx, bh + 4.0f, 2);
        if (fd_ui_button_pill(ctx, step_btn, cur_s < tot_s, nk_rgba(52, 199, 133, 255))) {
            fd_app_step_recipe_next(app);
            sv = fd_app_vessel(app);
        }
        if (fd_ui_button_styled(ctx, T(app, "btn_simulate_all"), true)) {
            if (app->kitchen.has_recipe) {
                fd_recipe_t copy_r = app->kitchen.recipe;
                fd_kitchen_run_recipe(&app->kitchen, &app->db, &app->lua, &copy_r, false);
                refresh_slice(app);
                app->pending_cue = FD_CUE_TIMER;
                set_status(app, "status_simulated", copy_r.name);
            }
        }
        fd_ui_tooltip(ctx, T(app, "tip_simulate"), app->touch_mode);

        /* Inline Executable Recipe Mise-en-Place & Detailed Steps */
        if (app->kitchen.has_recipe) {
            const fd_recipe_t *r = &app->kitchen.recipe;
            if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
            nk_layout_row_dynamic(ctx, 20.0f, 1);
            nk_label_colored(ctx, T(app, "sec_steps"), NK_TEXT_LEFT, nk_rgba(166, 190, 224, 255));
            for (int s = 0; s < r->step_count; s++) {
                char instr[200], line[224];
                fd_recipe_describe_step(r, &app->db, s, instr, sizeof(instr), NULL, 0);
                const char *mark = (s < app->kitchen.current_step) ? "[OK]"
                                 : ((s == app->kitchen.current_step) ? "[>>]" : "    ");
                snprintf(line, sizeof(line), "%s %d. %s", mark, s + 1, instr);
                struct nk_color col = (s < app->kitchen.current_step) ? nk_rgba(116, 214, 152, 255)
                                    : ((s == app->kitchen.current_step) ? nk_rgba(248, 210, 102, 255)
                                    : nk_rgba(204, 214, 232, 255));
                nk_layout_row_dynamic(ctx, 36.0f, 1);
                nk_label_colored_wrap(ctx, line, col);
            }
            nk_layout_row_dynamic(ctx, 20.0f, 1);
            nk_label_colored(ctx, T(app, "sec_ingredients"), NK_TEXT_LEFT, nk_rgba(166, 190, 224, 255));
            for (int i = 0; i < r->item_count; i++) {
                int ii = fd_db_find_ingredient(&app->db, r->items[i].ingredient_id);
                const char *nm = (ii >= 0) ? app->db.ingredients[ii].name : r->items[i].ingredient_id;
                int vi = (r->items[i].vessel_idx >= 0 && r->items[i].vessel_idx < r->vessel_count)
                       ? r->items[i].vessel_idx : 0;
                const char *vn = (r->vessel_count > 0) ? r->vessels[vi].name : "Main";
                char row[128];
                if (r->items[i].prep_note[0]) {
                    snprintf(row, sizeof(row), "• [%.12s] %.0fg %s (%s)",
                             vn, r->items[i].mass_g, nm, r->items[i].prep_note);
                } else {
                    snprintf(row, sizeof(row), "• [%.12s] %.0fg %s", vn, r->items[i].mass_g, nm);
                }
                nk_layout_row_dynamic(ctx, 18.0f, 1);
                nk_label(ctx, row, NK_TEXT_LEFT);
            }
            if (app->ui.font_small) nk_style_pop_font(ctx);
        }
    }
    nk_end(ctx);
}

static void render_slice_panel(fd_app_t *app, float x, float y, float w, float h) {
    struct nk_context *ctx = &app->ui.nk;
    float bh = app->touch_mode ? 48.0f : 30.0f;
    const fd_sim_state_t *sv = fd_app_vessel_const(app);

    if (nk_begin(ctx, "fd_slice", nk_rect(x, y, w, h), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
        char stitle[112];
        snprintf(stitle, sizeof(stitle), "%s — %s", T(app, "sec_slice"), sv->vessel_name[0] ? sv->vessel_name : "Station");
        draw_section_title(app, stitle);
        if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
        nk_layout_row_dynamic(ctx, 18.0f, 1);
        nk_label_colored(ctx, T(app, "slice_hint"), NK_TEXT_LEFT, nk_rgba(160, 176, 204, 255));
        if (app->ui.font_small) nk_style_pop_font(ctx);

        /* Plane & snap row */
        if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
        nk_layout_row_dynamic(ctx, bh, 5);
        if (fd_ui_button_styled(ctx, T(app, "plane_xy"), app->slice_plane == FD_SLICE_XY)) {
            app->slice_plane = FD_SLICE_XY; refresh_slice(app);
        }
        if (fd_ui_button_styled(ctx, T(app, "plane_xz"), app->slice_plane == FD_SLICE_XZ)) {
            app->slice_plane = FD_SLICE_XZ; refresh_slice(app);
        }
        if (fd_ui_button_styled(ctx, T(app, "plane_yz"), app->slice_plane == FD_SLICE_YZ)) {
            app->slice_plane = FD_SLICE_YZ; refresh_slice(app);
        }
        if (fd_ui_button_pill(ctx, T(app, "btn_cold_spot"), true, nk_rgba(77, 163, 245, 255))) {
            fd_app_snap_cold_spot(app);
        }
        if (fd_ui_button_pill(ctx, T(app, "btn_peak_crust"), true, nk_rgba(242, 134, 62, 255))) {
            fd_app_snap_hot_spot(app);
        }

        /* 5 Overlay mode pills */
        const char *ov_keys[5] = { "ov_temp", "ov_moist", "ov_brown", "ov_protein", "ov_safety" };
        nk_layout_row_dynamic(ctx, bh, 5);
        for (int i = 0; i < 5; i++) {
            if (fd_ui_button_styled(ctx, T(app, ov_keys[i]), (int)app->overlay_mode == i)) {
                app->overlay_mode = (fd_overlay_mode_t)i;
                refresh_slice(app);
            }
        }
        if (app->ui.font_small) nk_style_pop_font(ctx);

        /* Slice depth slider + physical dimensions indicator */
        const fd_grid_t *g = &sv->grid;
        int dim_n = (app->slice_plane == FD_SLICE_XY) ? g->nz
                  : ((app->slice_plane == FD_SLICE_XZ) ? g->ny : g->nx);
        int cur_n = (int)roundf(app->slice_norm * (float)(dim_n > 1 ? dim_n - 1 : 0)) + 1;
        char dbuf[72];
        snprintf(dbuf, sizeof(dbuf), "%.0fx%.0fx%.0fmm | %d/%d",
                 g->lx * 1000.0f, g->ly * 1000.0f, g->lz * 1000.0f,
                 cur_n, dim_n > 0 ? dim_n : 1);

        nk_layout_row_begin(ctx, NK_DYNAMIC, 20.0f, 2);
        nk_layout_row_push(ctx, 0.45f);
        nk_label(ctx, T(app, "lbl_slice_depth"), NK_TEXT_LEFT);
        nk_layout_row_push(ctx, 0.55f);
        nk_label_colored(ctx, dbuf, NK_TEXT_RIGHT, nk_rgba(182, 204, 236, 255));
        nk_layout_row_end(ctx);

        float prev_norm = app->slice_norm;
        nk_layout_row_dynamic(ctx, app->touch_mode ? 36.0f : 22.0f, 1);
        nk_slider_float(ctx, 0.0f, &app->slice_norm, 1.0f, 0.01f);
        if (fabsf(app->slice_norm - prev_norm) > 1e-4f) refresh_slice(app);

        /* Cross-section image filling remaining vertical space */
        float overhead = (app->touch_mode ? 245.0f : 200.0f) + 52.0f;
        float slice_h  = fmaxf(120.0f, h - overhead);
        fd_ui_draw_slice_box(ctx, sv, app->slice_plane, app->slice_norm, slice_h);

        /* Smooth colormap legend & crosshair key */
        nk_layout_row_dynamic(ctx, 44.0f, 1);
        fd_ui_draw_legend_bar(ctx, app->overlay_mode, T(app, "legend_cold_spot"), T(app, "legend_peak_crust"));
    }
    nk_end(ctx);
}

static void render_telemetry_panel(fd_app_t *app, float x, float y, float w, float h) {
    struct nk_context *ctx = &app->ui.nk;
    const fd_sim_state_t *sv = fd_app_vessel_const(app);
    const fd_metrics_t *m = &sv->metrics;

    if (nk_begin(ctx, "fd_telemetry", nk_rect(x, y, w, h), NK_WINDOW_BORDER)) {
        draw_section_title(app, T(app, "sec_telemetry"));

        /* Headline Score, Elapsed Time & Safety Pill */
        char score_s[48], time_s[48];
        snprintf(score_s, sizeof(score_s), "%s %.1f/100", T(app, "lbl_score"), m->overall_score);
        snprintf(time_s, sizeof(time_s), "%s %.0fs", T(app, "lbl_time"), app->kitchen.wall_time_sec);
        nk_layout_row_dynamic(ctx, 30.0f, 3);
        fd_ui_badge(ctx, score_s, nk_rgba(235, 166, 52, 255), nk_rgba(16, 20, 28, 255));
        fd_ui_badge(ctx, time_s, nk_rgba(40, 50, 70, 255), nk_rgba(228, 236, 248, 255));
        fd_ui_badge(ctx, m->is_pasteurized_safe ? T(app, "badge_safe") : T(app, "badge_raw"),
                    m->is_pasteurized_safe ? nk_rgba(46, 186, 122, 255) : nk_rgba(224, 78, 72, 255),
                    nk_rgba(14, 18, 26, 255));

        /* Thermal, Chemistry & Mechanical Structure bars */
        if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
        nk_layout_row_dynamic(ctx, 20.0f, 1);
        nk_label_colored(ctx, T(app, "sec_chemistry"), NK_TEXT_LEFT, nk_rgba(166, 190, 224, 255));

        char vbuf[48];
        nk_layout_row_dynamic(ctx, 25.0f, 2);
        snprintf(vbuf, sizeof(vbuf), "%.1f°C", m->surface_max_temp_c);
        fd_ui_metric_bar(ctx, T(app, "lbl_surface_temp"), vbuf, m->surface_max_temp_c / 250.0f, nk_rgba(246, 130, 58, 255));
        snprintf(vbuf, sizeof(vbuf), "%.1f°C (%.1fD)", m->cold_spot_temp_c, m->cold_spot_lethality_log10);
        fd_ui_metric_bar(ctx, T(app, "lbl_cold_temp"), vbuf, m->cold_spot_temp_c / 100.0f, nk_rgba(77, 163, 245, 255));

        nk_layout_row_dynamic(ctx, 25.0f, 2);
        snprintf(vbuf, sizeof(vbuf), "%.0f%%", m->crust_maillard * 100.0f);
        fd_ui_metric_bar(ctx, T(app, "lbl_maillard"), vbuf, m->crust_maillard, nk_rgba(235, 178, 66, 255));
        snprintf(vbuf, sizeof(vbuf), "%.0f%%", m->max_char * 100.0f);
        fd_ui_metric_bar(ctx, T(app, "lbl_char"), vbuf, m->max_char, nk_rgba(232, 84, 76, 255));

        nk_layout_row_dynamic(ctx, 25.0f, 3);
        snprintf(vbuf, sizeof(vbuf), "%.0f%%", m->gelatin_converted * 100.0f);
        fd_ui_metric_bar(ctx, T(app, "lbl_gelatin"), vbuf, m->gelatin_converted, nk_rgba(156, 128, 240, 255));
        snprintf(vbuf, sizeof(vbuf), "%.0f%%", m->starch_gelatinized * 100.0f);
        fd_ui_metric_bar(ctx, T(app, "lbl_starch"), vbuf, m->starch_gelatinized, nk_rgba(98, 206, 188, 255));
        snprintf(vbuf, sizeof(vbuf), "%.2f", m->ph);
        fd_ui_metric_bar(ctx, T(app, "lbl_ph"), vbuf, m->ph / 14.0f, nk_rgba(130, 214, 120, 255));

        /* Mechanical Movement & Non-Thermal Structure */
        nk_layout_row_dynamic(ctx, 25.0f, 3);
        snprintf(vbuf, sizeof(vbuf), "%.0f%%", m->avg_porosity * 100.0f);
        fd_ui_metric_bar(ctx, T(app, "lbl_foam"), vbuf, m->avg_porosity / 0.75f, nk_rgba(244, 198, 96, 255));
        float struct_v = fmaxf(sv->gluten_network, sv->mech_tender);
        snprintf(vbuf, sizeof(vbuf), "%.0f%%", struct_v * 100.0f);
        fd_ui_metric_bar(ctx, T(app, "lbl_gluten"), vbuf, struct_v, nk_rgba(176, 146, 248, 255));
        float rest_v = fmaxf(sv->rested_frac, fmaxf(sv->fond_maillard, sv->cell_rupture));
        snprintf(vbuf, sizeof(vbuf), "%.0f%%", rest_v * 100.0f);
        fd_ui_metric_bar(ctx, T(app, "lbl_fond_rest"), vbuf, rest_v, nk_rgba(92, 212, 162, 255));

        /* Sensory: Taste & Texture */
        nk_layout_row_dynamic(ctx, 20.0f, 1);
        nk_label_colored(ctx, T(app, "sec_sensory"), NK_TEXT_LEFT, nk_rgba(166, 190, 224, 255));

        nk_layout_row_dynamic(ctx, 24.0f, 3);
        snprintf(vbuf, sizeof(vbuf), "%.1f", m->umami);
        fd_ui_metric_bar(ctx, T(app, "lbl_umami"), vbuf, m->umami / 10.0f, nk_rgba(235, 166, 52, 255));
        snprintf(vbuf, sizeof(vbuf), "%.1f", m->salty);
        fd_ui_metric_bar(ctx, T(app, "lbl_salty"), vbuf, m->salty / 10.0f, nk_rgba(136, 192, 242, 255));
        snprintf(vbuf, sizeof(vbuf), "%.1f", m->sweet);
        fd_ui_metric_bar(ctx, T(app, "lbl_sweet"), vbuf, m->sweet / 10.0f, nk_rgba(242, 134, 178, 255));

        nk_layout_row_dynamic(ctx, 24.0f, 3);
        snprintf(vbuf, sizeof(vbuf), "%.1f", m->sour);
        fd_ui_metric_bar(ctx, T(app, "lbl_sour"), vbuf, m->sour / 10.0f, nk_rgba(176, 222, 82, 255));
        snprintf(vbuf, sizeof(vbuf), "%.1f", m->bitter);
        fd_ui_metric_bar(ctx, T(app, "lbl_bitter"), vbuf, m->bitter / 10.0f, nk_rgba(196, 112, 92, 255));
        snprintf(vbuf, sizeof(vbuf), "%.0f%%", m->emulsion_stability * 100.0f);
        fd_ui_metric_bar(ctx, T(app, "lbl_emulsion"), vbuf, m->emulsion_stability, nk_rgba(110, 214, 196, 255));

        nk_layout_row_dynamic(ctx, 24.0f, 3);
        snprintf(vbuf, sizeof(vbuf), "%.1f", m->crispness);
        fd_ui_metric_bar(ctx, T(app, "lbl_crisp"), vbuf, m->crispness / 10.0f, nk_rgba(246, 152, 68, 255));
        snprintf(vbuf, sizeof(vbuf), "%.1f", m->tenderness);
        fd_ui_metric_bar(ctx, T(app, "lbl_tender"), vbuf, m->tenderness / 10.0f, nk_rgba(92, 212, 150, 255));
        snprintf(vbuf, sizeof(vbuf), "%.1f", m->juiciness);
        fd_ui_metric_bar(ctx, T(app, "lbl_juicy"), vbuf, m->juiciness / 10.0f, nk_rgba(78, 174, 245, 255));

        /* Nutrition & Digestion */
        nk_layout_row_dynamic(ctx, 20.0f, 1);
        nk_label_colored(ctx, T(app, "sec_nutrition"), NK_TEXT_LEFT, nk_rgba(166, 190, 224, 255));
        char n1[140], n2[140];
        snprintf(n1, sizeof(n1), "%s: %.0f kcal (%.0f/100g) | P:%.1fg  F:%.1fg  C:%.1fg",
                 T(app, "lbl_calories"), m->kcal_total, m->kcal_per_100g,
                 m->protein_g, m->fat_g, m->net_carbs_g);
        snprintf(n2, sizeof(n2), "%s: %.1fg | %s: %.0f | %s: %.1f | %s: %.0f%%",
                 T(app, "lbl_res_starch"), m->resistant_starch_g,
                 T(app, "lbl_egi"), m->estimated_gi,
                 T(app, "lbl_egl"), m->estimated_gl,
                 T(app, "lbl_bioavail"), m->protein_bioavailability * 100.0f);
        nk_layout_row_dynamic(ctx, 18.0f, 1);
        nk_label_colored(ctx, n1, NK_TEXT_LEFT, nk_rgba(182, 232, 186, 255));
        nk_layout_row_dynamic(ctx, 18.0f, 1);
        nk_label_colored(ctx, n2, NK_TEXT_LEFT, nk_rgba(182, 232, 186, 255));

        /* Live Multi-Curve Plot */
        nk_layout_row_dynamic(ctx, 20.0f, 1);
        nk_label_colored(ctx, T(app, "sec_curves"), NK_TEXT_LEFT, nk_rgba(166, 190, 224, 255));
        fd_ui_draw_thermal_chart(ctx, sv, T(app, "curve_surf"), T(app, "curve_cold"),
                                 T(app, "curve_maillard"), fmaxf(110.0f, h * 0.22f));
        if (app->ui.font_small) nk_style_pop_font(ctx);
    }
    nk_end(ctx);
}

static void render_diet_filters(fd_app_t *app, float bh) {
    struct nk_context *ctx = &app->ui.nk;
    bool is_vegan = (app->constraints.required_diet_flags & FD_DIET_VEGAN) != 0;
    bool is_gf    = (app->constraints.required_diet_flags & FD_DIET_GLUTEN_FREE) != 0;
    bool is_df    = (app->constraints.required_diet_flags & FD_DIET_DAIRY_FREE) != 0;
    bool low_gi   = (app->constraints.max_gi > 0.0f && app->constraints.max_gi <= 55.0f);

    nk_layout_row_dynamic(ctx, bh, app->touch_mode ? 2 : 4);
    if (fd_ui_button_pill(ctx, T(app, "diet_vegan"), is_vegan, nk_rgba(52, 199, 133, 255))) {
        app->constraints.required_diet_flags ^= FD_DIET_VEGAN;
    }
    if (fd_ui_button_pill(ctx, T(app, "diet_gf"), is_gf, nk_rgba(52, 199, 133, 255))) {
        app->constraints.required_diet_flags ^= FD_DIET_GLUTEN_FREE;
    }
    if (fd_ui_button_pill(ctx, T(app, "diet_df"), is_df, nk_rgba(52, 199, 133, 255))) {
        app->constraints.required_diet_flags ^= FD_DIET_DAIRY_FREE;
    }
    if (fd_ui_button_pill(ctx, T(app, "diet_low_gi"), low_gi, nk_rgba(77, 163, 245, 255))) {
        app->constraints.max_gi = low_gi ? 0.0f : 52.0f;
    }
}

static void render_propose_tab(fd_app_t *app, float x, float y, float w, float h) {
    struct nk_context *ctx = &app->ui.nk;
    float bh = app->touch_mode ? 48.0f : 34.0f;

    if (nk_begin(ctx, "fd_propose", nk_rect(x, y, w, h), NK_WINDOW_BORDER)) {
        draw_section_title(app, T(app, "propose_title"));
        nk_layout_row_dynamic(ctx, app->touch_mode ? 42.0f : 24.0f, 1);
        nk_label_colored_wrap(ctx, T(app, "propose_desc"), nk_rgba(176, 192, 216, 255));

        render_diet_filters(app, bh);

        char tmax_s[48];
        snprintf(tmax_s, sizeof(tmax_s), "%s: %.0fs", T(app, "lbl_max_time"), app->constraints.max_time_sec);
        nk_layout_row_begin(ctx, NK_DYNAMIC, bh, 3);
        nk_layout_row_push(ctx, 0.24f);
        nk_label(ctx, tmax_s, NK_TEXT_LEFT);
        nk_layout_row_push(ctx, 0.42f);
        nk_slider_float(ctx, 180.0f, &app->constraints.max_time_sec, 9000.0f, 60.0f);
        nk_layout_row_push(ctx, 0.34f);
        if (fd_ui_button_styled(ctx, T(app, "btn_match_classics"), true)) {
            app->proposal_count = fd_solver_propose_classics(&app->db, &app->lua, &app->constraints,
                                                             app->proposals, 5);
            app->pending_cue = FD_CUE_TIMER;
            char cnt[16];
            snprintf(cnt, sizeof(cnt), "%d", app->proposal_count);
            set_status(app, "status_proposed", cnt);
        }
        nk_layout_row_end(ctx);

        nk_layout_row_dynamic(ctx, 10.0f, 1);
        nk_spacing(ctx, 1);

        if (app->proposal_count == 0) {
            nk_layout_row_dynamic(ctx, 40.0f, 1);
            nk_label_colored_wrap(ctx, T(app, "propose_empty"), nk_rgba(154, 170, 196, 255));
        }

        for (int i = 0; i < app->proposal_count; i++) {
            const fd_proposal_t *p = &app->proposals[i];
            char gname[32];
            snprintf(gname, sizeof(gname), "prop_card_%d", i);
            nk_layout_row_dynamic(ctx, app->touch_mode ? 122.0f : 92.0f, 1);
            if (nk_group_begin(ctx, gname, NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
                char hdr[148];
                snprintf(hdr, sizeof(hdr), "#%d  %s  [%d %s, %d %s | %.1f/100 | eGI %.0f]",
                         i + 1, p->recipe.name,
                         p->recipe.vessel_count > 0 ? p->recipe.vessel_count : 1, T(app, "lbl_vessels"),
                         p->recipe.step_count, T(app, "sec_steps"),
                         p->metrics.overall_score, p->metrics.estimated_gi);
                nk_layout_row_begin(ctx, NK_DYNAMIC, bh, 2);
                nk_layout_row_push(ctx, 0.76f);
                nk_label_colored(ctx, hdr, NK_TEXT_LEFT, nk_rgba(248, 212, 114, 255));
                nk_layout_row_push(ctx, 0.24f);
                if (fd_ui_button_styled(ctx, T(app, "btn_cook_dish"), true)) {
                    fd_app_load_recipe(app, &p->recipe);
                    app->active_tab = FD_TAB_WORKBENCH;
                }
                nk_layout_row_end(ctx);
                if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
                nk_layout_row_dynamic(ctx, 20.0f, 1);
                nk_label_colored(ctx, p->rationale, NK_TEXT_LEFT, nk_rgba(188, 204, 228, 255));
                char s0[160];
                fd_recipe_describe_step(&p->recipe, &app->db, 0, s0, sizeof(s0), NULL, 0);
                nk_layout_row_dynamic(ctx, 20.0f, 1);
                nk_label_colored(ctx, s0, NK_TEXT_LEFT, nk_rgba(150, 214, 172, 255));
                if (app->ui.font_small) nk_style_pop_font(ctx);
                nk_group_end(ctx);
            }
        }
    }
    nk_end(ctx);
}

static void render_invent_tab(fd_app_t *app, float x, float y, float w, float h) {
    struct nk_context *ctx = &app->ui.nk;
    float bh = app->touch_mode ? 48.0f : 34.0f;

    if (nk_begin(ctx, "fd_invent", nk_rect(x, y, w, h), NK_WINDOW_BORDER)) {
        draw_section_title(app, T(app, "invent_title"));
        nk_layout_row_dynamic(ctx, app->touch_mode ? 42.0f : 24.0f, 1);
        nk_label_colored_wrap(ctx, T(app, "invent_desc"), nk_rgba(176, 192, 216, 255));

        render_diet_filters(app, bh);

        if (app->constraints.target_crispness < 0.0f) app->constraints.target_crispness = 6.5f;
        if (app->constraints.target_umami < 0.0f)     app->constraints.target_umami     = 7.0f;

        char l1[48], l2[48], l3[48];
        snprintf(l1, sizeof(l1), "%s: %.1f", T(app, "lbl_target_crisp"), app->constraints.target_crispness);
        snprintf(l2, sizeof(l2), "%s: %.1f", T(app, "lbl_target_umami"), app->constraints.target_umami);
        snprintf(l3, sizeof(l3), "%s: %.0fg", T(app, "lbl_min_protein"), app->constraints.min_protein_g);

        nk_layout_row_dynamic(ctx, 20.0f, app->touch_mode ? 1 : 3);
        nk_label(ctx, l1, NK_TEXT_LEFT);
        if (!app->touch_mode) {
            nk_label(ctx, l2, NK_TEXT_LEFT);
            nk_label(ctx, l3, NK_TEXT_LEFT);
        }
        nk_layout_row_dynamic(ctx, app->touch_mode ? 36.0f : 24.0f, app->touch_mode ? 1 : 3);
        nk_slider_float(ctx, 0.0f, &app->constraints.target_crispness, 10.0f, 0.2f);
        if (app->touch_mode) {
            nk_layout_row_dynamic(ctx, 20.0f, 1);
            nk_label(ctx, l2, NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 36.0f, 1);
        }
        nk_slider_float(ctx, 0.0f, &app->constraints.target_umami, 10.0f, 0.2f);
        if (app->touch_mode) {
            nk_layout_row_dynamic(ctx, 20.0f, 1);
            nk_label(ctx, l3, NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 36.0f, 1);
        }
        nk_slider_float(ctx, 0.0f, &app->constraints.min_protein_g, 55.0f, 1.0f);

        nk_layout_row_dynamic(ctx, bh, 2);
        if (fd_ui_button_pill(ctx, app->constraints.require_emulsion ? T(app, "btn_emul_on") : T(app, "btn_emul_off"),
                              app->constraints.require_emulsion, nk_rgba(77, 163, 245, 255))) {
            app->constraints.require_emulsion = !app->constraints.require_emulsion;
        }
        if (fd_ui_button_styled(ctx, T(app, "btn_invent_go"), true)) {
            app->constraints.rng_seed += 1337u;
            fd_inventor_start(&app->inventor, &app->db, &app->constraints);
            set_status(app, "status_inventing", NULL);
        }

        /* Progress bar */
        nk_size prog = (nk_size)(fd_inventor_progress(&app->inventor) * 100.0f);
        nk_layout_row_dynamic(ctx, 18.0f, 1);
        nk_progress(ctx, &prog, 100, NK_FIXED);

        if (app->inventor.active && !app->inventor.done) {
            nk_layout_row_dynamic(ctx, 22.0f, 1);
            nk_label_colored(ctx, T(app, "invent_running"), NK_TEXT_LEFT, nk_rgba(248, 206, 98, 255));
        }

        if (app->inventor.done || app->inventor.evals_done > 0) {
            const fd_proposal_t *b = &app->inventor.best;
            nk_layout_row_dynamic(ctx, fmaxf(230.0f, h * 0.46f), 1);
            if (nk_group_begin(ctx, "inv_result", NK_WINDOW_BORDER)) {
                char hdr[180];
                snprintf(hdr, sizeof(hdr), "%s   [%d %s | %.1f/100 | %s | %.0f kcal]",
                         b->recipe.name,
                         b->recipe.vessel_count > 0 ? b->recipe.vessel_count : 1, T(app, "lbl_vessels"),
                         b->metrics.overall_score,
                         b->metrics.is_pasteurized_safe ? T(app, "badge_safe") : T(app, "badge_raw"),
                         b->metrics.kcal_total);
                nk_layout_row_dynamic(ctx, 26.0f, 1);
                nk_label_colored(ctx, hdr, NK_TEXT_LEFT, nk_rgba(88, 226, 154, 255));
                nk_layout_row_dynamic(ctx, 22.0f, 1);
                nk_label_colored(ctx, b->rationale, NK_TEXT_LEFT, nk_rgba(190, 206, 230, 255));

                for (int i = 0; i < b->recipe.item_count; i++) {
                    int idx = fd_db_find_ingredient(&app->db, b->recipe.items[i].ingredient_id);
                    const char *nm = (idx >= 0) ? app->db.ingredients[idx].name : b->recipe.items[i].ingredient_id;
                    int vi = (b->recipe.items[i].vessel_idx >= 0 && b->recipe.items[i].vessel_idx < b->recipe.vessel_count)
                           ? b->recipe.items[i].vessel_idx : 0;
                    const char *vn = (b->recipe.vessel_count > 0) ? b->recipe.vessels[vi].name : "Station";
                    char line[220];
                    snprintf(line, sizeof(line), "• [%s] %.1f g  %s (%s)",
                             vn, b->recipe.items[i].mass_g, nm, b->recipe.items[i].prep_note);
                    nk_layout_row_dynamic(ctx, 18.0f, 1);
                    nk_label(ctx, line, NK_TEXT_LEFT);
                }
                for (int s = 0; s < b->recipe.step_count; s++) {
                    char instr[200], why[160], line[230];
                    fd_recipe_describe_step(&b->recipe, &app->db, s, instr, sizeof(instr), why, sizeof(why));
                    snprintf(line, sizeof(line), "%d. %s", s + 1, instr);
                    nk_layout_row_dynamic(ctx, 22.0f, 1);
                    nk_label_colored_wrap(ctx, line, nk_rgba(246, 206, 118, 255));
                    if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
                    nk_layout_row_dynamic(ctx, 18.0f, 1);
                    nk_label_colored(ctx, why, NK_TEXT_LEFT, nk_rgba(154, 186, 224, 255));
                    if (app->ui.font_small) nk_style_pop_font(ctx);
                }
                if (app->inventor.done) {
                    nk_layout_row_dynamic(ctx, bh, 1);
                    if (fd_ui_button_styled(ctx, T(app, "btn_load_invented"), true)) {
                        fd_app_load_recipe(app, &b->recipe);
                        app->active_tab = FD_TAB_WORKBENCH;
                    }
                }
                nk_group_end(ctx);
            }
        } else {
            nk_layout_row_dynamic(ctx, 36.0f, 1);
            nk_label_colored_wrap(ctx, T(app, "invent_empty"), nk_rgba(154, 170, 196, 255));
        }
    }
    nk_end(ctx);
}

static void render_mods_tab(fd_app_t *app, float x, float y, float w, float h) {
    struct nk_context *ctx = &app->ui.nk;
    float bh = app->touch_mode ? 48.0f : 36.0f;

    if (nk_begin(ctx, "fd_mods", nk_rect(x, y, w, h), NK_WINDOW_BORDER)) {
        draw_section_title(app, T(app, "mods_title"));
        nk_layout_row_dynamic(ctx, app->touch_mode ? 48.0f : 26.0f, 1);
        nk_label_colored_wrap(ctx, T(app, "mods_desc"), nk_rgba(176, 192, 216, 255));

        char b1[48], b2[48], b3[48], b4[48];
        snprintf(b1, sizeof(b1), "%s: %d", T(app, "lbl_count_ing"), app->db.ingredient_count);
        snprintf(b2, sizeof(b2), "%s: %d", T(app, "lbl_count_tech"), app->db.technique_count);
        snprintf(b3, sizeof(b3), "%s: %d", T(app, "lbl_count_rec"), app->db.recipe_count);
        snprintf(b4, sizeof(b4), "%s: %d", T(app, "lbl_count_mods"), app->lua.mod_count);
        nk_layout_row_dynamic(ctx, 32.0f, app->touch_mode ? 2 : 4);
        fd_ui_badge(ctx, b1, nk_rgba(36, 46, 66, 255), nk_rgba(232, 238, 248, 255));
        fd_ui_badge(ctx, b2, nk_rgba(36, 46, 66, 255), nk_rgba(232, 238, 248, 255));
        fd_ui_badge(ctx, b3, nk_rgba(36, 46, 66, 255), nk_rgba(232, 238, 248, 255));
        fd_ui_badge(ctx, b4, nk_rgba(52, 199, 133, 255), nk_rgba(14, 18, 26, 255));

        char dir_line[220];
        snprintf(dir_line, sizeof(dir_line), "%s: %.150s", T(app, "lbl_assets_dir"), app->vfs.base_dir);
        nk_layout_row_dynamic(ctx, 24.0f, 1);
        nk_label_colored(ctx, dir_line, NK_TEXT_LEFT, nk_rgba(182, 204, 236, 255));

        nk_layout_row_dynamic(ctx, bh, app->touch_mode ? 1 : 2);
        if (fd_ui_button_styled(ctx, T(app, "btn_hot_reload"), true)) {
            app->pending_reload = true;
        }
        if (fd_ui_button_pill(ctx, app->mod_boost_enabled ? T(app, "btn_truffle_on") : T(app, "btn_truffle_off"),
                              app->mod_boost_enabled, nk_rgba(52, 199, 133, 255))) {
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
            set_status(app, app->mod_boost_enabled ? "status_mod_on" : "status_mod_off", NULL);
        }

        nk_layout_row_dynamic(ctx, 40.0f, 1);
        nk_label_colored_wrap(ctx, T(app, "mods_tip"), nk_rgba(164, 182, 210, 255));
    }
    nk_end(ctx);
}

static void render_recipe_card_modal(fd_app_t *app, float W, float H) {
    if (!app->recipe_card_open || !app->kitchen.has_recipe) return;
    struct nk_context *ctx = &app->ui.nk;
    const fd_recipe_t *r = &app->kitchen.recipe;
    float mw = app->touch_mode ? (W - 14.0f) : fminf(840.0f, W * 0.82f);
    float mh = app->touch_mode ? (H - 86.0f) : fminf(580.0f, H * 0.82f);
    float mx = (W - mw) * 0.5f;
    float my = (H - mh) * 0.5f;

    ctx->style.window.border_color = nk_rgba(235, 166, 52, 255);
    ctx->style.window.border       = 2.0f;
    if (nk_begin(ctx, "fd_recipe_card", nk_rect(mx, my, mw, mh), NK_WINDOW_BORDER)) {
        nk_layout_row_begin(ctx, NK_DYNAMIC, 30.0f, 2);
        nk_layout_row_push(ctx, 0.82f);
        if (app->ui.font_title) nk_style_push_font(ctx, &app->ui.uf_title);
        nk_label_colored(ctx, r->name, NK_TEXT_LEFT, nk_rgba(248, 206, 92, 255));
        if (app->ui.font_title) nk_style_pop_font(ctx);
        nk_layout_row_push(ctx, 0.18f);
        if (fd_ui_button_styled(ctx, T(app, "btn_close"), false)) app->recipe_card_open = false;
        nk_layout_row_end(ctx);

        if (r->notes[0] != '\0') {
            nk_layout_row_dynamic(ctx, app->touch_mode ? 44.0f : 26.0f, 1);
            nk_label_colored_wrap(ctx, r->notes, nk_rgba(196, 212, 236, 255));
        }

        /* Vessels / Equipment section */
        nk_layout_row_dynamic(ctx, 22.0f, 1);
        nk_label_colored(ctx, T(app, "sec_vessels"), NK_TEXT_LEFT, nk_rgba(88, 216, 152, 255));
        for (int v = 0; v < r->vessel_count; v++) {
            char vline[128];
            snprintf(vline, sizeof(vline), "%d. %s  (%.0fx%.0fx%.0f mm)",
                     v + 1, r->vessels[v].name, r->vessels[v].lx_mm, r->vessels[v].ly_mm, r->vessels[v].lz_mm);
            nk_layout_row_dynamic(ctx, 19.0f, 1);
            nk_label(ctx, vline, NK_TEXT_LEFT);
        }

        /* Mise-en-Place Ingredients */
        nk_layout_row_dynamic(ctx, 22.0f, 1);
        nk_label_colored(ctx, T(app, "sec_ingredients"), NK_TEXT_LEFT, nk_rgba(88, 216, 152, 255));
        for (int i = 0; i < r->item_count; i++) {
            int ii = fd_db_find_ingredient(&app->db, r->items[i].ingredient_id);
            const char *nm = (ii >= 0) ? app->db.ingredients[ii].name : r->items[i].ingredient_id;
            int vi = (r->items[i].vessel_idx >= 0 && r->items[i].vessel_idx < r->vessel_count) ? r->items[i].vessel_idx : 0;
            char iline[220];
            snprintf(iline, sizeof(iline), "• %5.1f g  %-26s  [%s%s%s]",
                     r->items[i].mass_g, nm, r->vessels[vi].name,
                     r->items[i].prep_note[0] ? " — " : "", r->items[i].prep_note);
            nk_layout_row_dynamic(ctx, 19.0f, 1);
            nk_label(ctx, iline, NK_TEXT_LEFT);
        }

        /* Detailed Executable Steps + First-Principles Rationale */
        nk_layout_row_dynamic(ctx, 24.0f, 1);
        nk_label_colored(ctx, T(app, "sec_steps"), NK_TEXT_LEFT, nk_rgba(248, 198, 86, 255));
        for (int s = 0; s < r->step_count; s++) {
            char instr[200], why[160], sline[232], wline[184];
            fd_recipe_describe_step(r, &app->db, s, instr, sizeof(instr), why, sizeof(why));
            snprintf(sline, sizeof(sline), "Step %d: %s", s + 1, instr);
            snprintf(wline, sizeof(wline), "   Why: %s", why);
            nk_layout_row_dynamic(ctx, app->touch_mode ? 54.0f : 34.0f, 1);
            nk_label_colored_wrap(ctx, sline, nk_rgba(238, 242, 250, 255));
            if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
            nk_layout_row_dynamic(ctx, app->touch_mode ? 40.0f : 22.0f, 1);
            nk_label_colored_wrap(ctx, wline, nk_rgba(148, 186, 232, 255));
            if (app->ui.font_small) nk_style_pop_font(ctx);
        }
    }
    nk_end(ctx);
    ctx->style.window.border_color = nk_rgba(50, 62, 84, 255);
    ctx->style.window.border       = 1.0f;
}

static void render_tutorial_overlay(fd_app_t *app, float W, float H) {
    if (!app->tutorial_open) return;
    struct nk_context *ctx = &app->ui.nk;
    float cw = fminf(W - 24.0f, 560.0f);
    float ch = app->touch_mode ? 255.0f : 215.0f;
    float cx = (W - cw) * 0.5f;
    float cy = H - ch - (app->touch_mode ? 90.0f : 38.0f);

    static const char *title_keys[FD_TUTORIAL_STEPS] = {
        "tut_1_title", "tut_2_title", "tut_3_title", "tut_4_title", "tut_5_title", "tut_6_title"
    };
    static const char *body_keys[FD_TUTORIAL_STEPS] = {
        "tut_1_body", "tut_2_body", "tut_3_body", "tut_4_body", "tut_5_body", "tut_6_body"
    };
    int st = app->tutorial_step;
    if (st < 0 || st >= FD_TUTORIAL_STEPS) st = 0;

    ctx->style.window.border_color = nk_rgba(235, 166, 52, 255);
    ctx->style.window.border       = 2.0f;
    if (nk_begin(ctx, "fd_tutorial", nk_rect(cx, cy, cw, ch), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_row_begin(ctx, NK_DYNAMIC, 24.0f, 2);
        nk_layout_row_push(ctx, 0.80f);
        nk_label_colored(ctx, T(app, title_keys[st]), NK_TEXT_LEFT, nk_rgba(248, 206, 92, 255));
        nk_layout_row_push(ctx, 0.20f);
        if (fd_ui_button_styled(ctx, T(app, "btn_close"), false)) {
            app->tutorial_open = false;
        }
        nk_layout_row_end(ctx);

        nk_size prog = (nk_size)(st + 1);
        nk_layout_row_dynamic(ctx, 10.0f, 1);
        nk_progress(ctx, &prog, FD_TUTORIAL_STEPS, NK_FIXED);

        nk_layout_row_dynamic(ctx, app->touch_mode ? 115.0f : 94.0f, 1);
        nk_label_colored_wrap(ctx, T(app, body_keys[st]), nk_rgba(228, 236, 248, 255));

        float bh = app->touch_mode ? 46.0f : 32.0f;
        nk_layout_row_dynamic(ctx, bh, 3);
        if (fd_ui_button_styled(ctx, T(app, "btn_tut_back"), false) && st > 0) {
            app->tutorial_step = st - 1;
        }
        if (fd_ui_button_pill(ctx, T(app, "btn_tut_try"), true, nk_rgba(52, 199, 133, 255))) {
            fd_app_tutorial_try_step(app, st);
        }
        if (st + 1 < FD_TUTORIAL_STEPS) {
            if (fd_ui_button_styled(ctx, T(app, "btn_tut_next"), true)) app->tutorial_step = st + 1;
        } else {
            if (fd_ui_button_styled(ctx, T(app, "btn_tut_done"), true)) app->tutorial_open = false;
        }
    }
    nk_end(ctx);
    ctx->style.window.border_color = nk_rgba(50, 62, 84, 255);
    ctx->style.window.border       = 1.0f;
}

static void render_help_modal(fd_app_t *app, float W, float H) {
    if (!app->help_open) return;
    struct nk_context *ctx = &app->ui.nk;
    float mw = app->touch_mode ? (W - 16.0f) : fminf(780.0f, W * 0.78f);
    float mh = app->touch_mode ? (H - 90.0f) : fminf(520.0f, H * 0.76f);
    float mx = (W - mw) * 0.5f;
    float my = (H - mh) * 0.5f;

    static const char *tab_keys[FD_HELP_TABS] = {
        "help_tab_start", "help_tab_science", "help_tab_safety", "help_tab_controls", "help_tab_modding"
    };
    static const char *body_keys[FD_HELP_TABS] = {
        "help_start_body", "help_science_body", "help_safety_body", "help_controls_body", "help_modding_body"
    };

    ctx->style.window.border_color = nk_rgba(77, 163, 245, 255);
    ctx->style.window.border       = 2.0f;
    if (nk_begin(ctx, "fd_help_modal", nk_rect(mx, my, mw, mh), NK_WINDOW_BORDER)) {
        nk_layout_row_begin(ctx, NK_DYNAMIC, 30.0f, 2);
        nk_layout_row_push(ctx, 0.82f);
        if (app->ui.font_title) nk_style_push_font(ctx, &app->ui.uf_title);
        nk_label_colored(ctx, T(app, "help_title"), NK_TEXT_LEFT, nk_rgba(248, 206, 92, 255));
        if (app->ui.font_title) nk_style_pop_font(ctx);
        nk_layout_row_push(ctx, 0.18f);
        if (fd_ui_button_styled(ctx, T(app, "btn_close"), false)) app->help_open = false;
        nk_layout_row_end(ctx);

        float bh = app->touch_mode ? 44.0f : 30.0f;
        nk_layout_row_dynamic(ctx, bh, app->touch_mode ? 2 : 5);
        for (int i = 0; i < FD_HELP_TABS; i++) {
            if (fd_ui_button_pill(ctx, T(app, tab_keys[i]), app->help_tab == i, nk_rgba(77, 163, 245, 255))) {
                app->help_tab = i;
            }
        }

        nk_layout_row_dynamic(ctx, 8.0f, 1);
        nk_spacing(ctx, 1);

        int ht = (app->help_tab >= 0 && app->help_tab < FD_HELP_TABS) ? app->help_tab : 0;
        const char *body = T(app, body_keys[ht]);
        const char *p = body;
        while (*p) {
            const char *nl = strchr(p, '\n');
            char para[384];
            size_t len = nl ? (size_t)(nl - p) : strlen(p);
            if (len >= sizeof(para)) len = sizeof(para) - 1;
            memcpy(para, p, len);
            para[len] = '\0';
            float lines = fmaxf(1.0f, ceilf((float)len / (app->touch_mode ? 34.0f : 72.0f)));
            nk_layout_row_dynamic(ctx, lines * 22.0f + 6.0f, 1);
            nk_label_colored_wrap(ctx, para, nk_rgba(226, 234, 246, 255));
            p = nl ? (nl + 1) : (p + len);
        }

        nk_layout_row_dynamic(ctx, 10.0f, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, bh + 2.0f, 2);
        if (fd_ui_button_pill(ctx, T(app, "btn_start_tour"), true, nk_rgba(52, 199, 133, 255))) {
            fd_app_start_tutorial(app);
        }
        if (fd_ui_button_styled(ctx, T(app, "btn_open_online_docs"), true)) {
            snprintf(app->pending_url, sizeof(app->pending_url), "%s", FD_ONLINE_DOCS_URL);
        }
    }
    nk_end(ctx);
    ctx->style.window.border_color = nk_rgba(50, 62, 84, 255);
    ctx->style.window.border       = 1.0f;
}

void fd_app_update_and_render(fd_app_t *app, const fd_input_t *in, float frame_dt_sec) {
    if (!app || !in) return;
    app->pending_cue = FD_CUE_NONE;

    if (!app->touch_manual_override) {
        if (in->is_touch_event || in->width < 820.0f) app->touch_mode = true;
    }

    fd_sim_state_t *sv = fd_app_vessel(app);

    /* Handle hotkeys */
    if (in->key_help) {
        app->help_open = !app->help_open;
        if (app->help_open) { app->tutorial_open = false; app->recipe_card_open = false; }
    }
    if (in->key_space) app->paused = !app->paused;
    if (in->key_flip && sv->initialized) {
        fd_grid_flip_z(&sv->grid);
        refresh_slice(app);
        app->pending_cue = FD_CUE_SEAR_DROP;
    }
    if (in->key_plane) {
        app->slice_plane = (fd_slice_plane_t)(((int)app->slice_plane + 1) % 3);
        refresh_slice(app);
    }
    if (in->key_reload) {
        fd_lua_reload_all(&app->lua);
        fd_i18n_set_lang(&app->i18n, &app->lua, app->i18n.active_lang);
        fd_ui_rebuild_fonts(&app->ui, &app->vfs, &app->i18n, app->ui.baked_dpi_scale);
        set_status(app, "status_reloaded", NULL);
    }
    if (in->speed_key >= 1 && in->speed_key <= 5) {
        static const float speeds[5] = { 1.0f, 5.0f, 15.0f, 45.0f, 120.0f };
        app->time_scale = speeds[in->speed_key - 1];
    }

    /* Step frame-sliced 3D evolutionary recipe inventor if active */
    if (app->inventor.active && !app->inventor.done) {
        if (fd_inventor_step(&app->inventor, &app->db, &app->lua, 12)) {
            app->pending_cue = FD_CUE_TIMER;
            set_status(app, "status_invented", app->inventor.best.recipe.name);
        }
    }

    /* Advance all active kitchen pots, skillets, baths & resting stations simultaneously */
    if (!app->paused && frame_dt_sec > 0.0f && app->active_tab == FD_TAB_WORKBENCH) {
        float sim_dt = clampf(frame_dt_sec, 0.0f, 0.1f) * app->time_scale;
        fd_kitchen_step_live(&app->kitchen, &app->db, &app->lua, sim_dt);
        refresh_slice(app);
    }

    /* Begin Nuklear frame */
    fd_ui_begin(&app->ui, in, app->touch_mode);
    struct nk_context *ctx = &app->ui.nk;
    float W = fmaxf(320.0f, in->width);
    float H = fmaxf(320.0f, in->height);

    const char *tab_keys[4] = { "tab_workbench", "tab_propose", "tab_invent", "tab_mods" };
    float top_h    = app->touch_mode ? 54.0f : 50.0f;
    float status_h = 26.0f;

    /* 1. Top Navigation & Utility Bar */
    if (nk_begin(ctx, "fd_header", nk_rect(0.0f, 0.0f, W, top_h), NK_WINDOW_NO_SCROLLBAR)) {
        char cur_lang_lbl[64];
        fd_i18n_lang_menu_label(&app->i18n, app->i18n.active_lang, cur_lang_lbl, sizeof(cur_lang_lbl));

        if (app->touch_mode) {
            char short_lang[16];
            const fd_lang_info_t *li = fd_i18n_lang_info(app->i18n.active_lang);
            snprintf(short_lang, sizeof(short_lang), "%c%c",
                     li->code[0] >= 'a' ? li->code[0] - 32 : li->code[0],
                     li->code[1] >= 'a' ? li->code[1] - 32 : li->code[1]);
            if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
            nk_layout_row_begin(ctx, NK_DYNAMIC, top_h - 16.0f, 5);
            nk_layout_row_push(ctx, 0.21f);
            nk_label_colored(ctx, T(app, "app_title"), NK_TEXT_LEFT, nk_rgba(248, 198, 76, 255));
            nk_layout_row_push(ctx, 0.20f);
            if (nk_combo_begin_label(ctx, short_lang, nk_vec2(220.0f, 320.0f))) {
                nk_layout_row_dynamic(ctx, 40.0f, 1);
                for (int i = 0; i < FD_LANG_COUNT; i++) {
                    char ml[64];
                    fd_i18n_lang_menu_label(&app->i18n, i, ml, sizeof(ml));
                    if (nk_combo_item_label(ctx, ml, NK_TEXT_LEFT)) app->pending_lang_idx = i;
                }
                nk_combo_end(ctx);
            }
            nk_layout_row_push(ctx, 0.21f);
            if (fd_ui_button_pill(ctx, T(app, "btn_tutorial"), app->tutorial_open, nk_rgba(52, 199, 133, 255))) {
                if (app->tutorial_open) app->tutorial_open = false;
                else fd_app_start_tutorial(app);
            }
            nk_layout_row_push(ctx, 0.19f);
            if (fd_ui_button_pill(ctx, T(app, "btn_help"), app->help_open, nk_rgba(77, 163, 245, 255))) {
                app->help_open = !app->help_open;
                if (app->help_open) { app->tutorial_open = false; app->recipe_card_open = false; }
            }
            nk_layout_row_push(ctx, 0.19f);
            if (fd_ui_button_styled(ctx, T(app, "mode_touch"), true)) {
                app->touch_mode = false;
                app->touch_manual_override = true;
            }
            nk_layout_row_end(ctx);
            if (app->ui.font_small) nk_style_pop_font(ctx);
        } else {
            float tab_w  = clampf((W - 680.0f) / 4.0f, 76.0f, 108.0f);
            float lang_w = 152.0f, tut_w = 88.0f, hlp_w = 72.0f, mod_w = 84.0f;
            float used_w = 34.0f + 98.0f + 4.0f * tab_w + lang_w + tut_w + hlp_w + mod_w + 96.0f;
            float sp_w   = fmaxf(4.0f, W - used_w);

            nk_layout_row_begin(ctx, NK_STATIC, top_h - 16.0f, 11);
            nk_layout_row_push(ctx, 34.0f);
            struct nk_image logo = nk_subimage_id((int)FD_TEX_KITCHEN, 512, 256,
                                                  nk_rect(384.0f, 128.0f, 128.0f, 128.0f));
            nk_image(ctx, logo);

            nk_layout_row_push(ctx, 98.0f);
            if (app->ui.font_title) nk_style_push_font(ctx, &app->ui.uf_title);
            nk_label_colored(ctx, T(app, "app_title"), NK_TEXT_LEFT, nk_rgba(248, 198, 76, 255));
            if (app->ui.font_title) nk_style_pop_font(ctx);

            for (int t = 0; t < 4; t++) {
                nk_layout_row_push(ctx, tab_w);
                if (fd_ui_button_styled(ctx, T(app, tab_keys[t]), (int)app->active_tab == t)) {
                    app->active_tab = (fd_tab_t)t;
                }
            }

            nk_layout_row_push(ctx, sp_w);
            nk_spacing(ctx, 1);

            nk_layout_row_push(ctx, lang_w);
            if (nk_combo_begin_label(ctx, cur_lang_lbl, nk_vec2(220.0f, 340.0f))) {
                nk_layout_row_dynamic(ctx, 26.0f, 1);
                for (int i = 0; i < FD_LANG_COUNT; i++) {
                    char ml[64];
                    fd_i18n_lang_menu_label(&app->i18n, i, ml, sizeof(ml));
                    if (nk_combo_item_label(ctx, ml, NK_TEXT_LEFT)) app->pending_lang_idx = i;
                }
                nk_combo_end(ctx);
            }

            nk_layout_row_push(ctx, tut_w);
            if (fd_ui_button_pill(ctx, T(app, "btn_tutorial"), app->tutorial_open, nk_rgba(52, 199, 133, 255))) {
                if (app->tutorial_open) app->tutorial_open = false;
                else fd_app_start_tutorial(app);
            }

            nk_layout_row_push(ctx, hlp_w);
            if (fd_ui_button_pill(ctx, T(app, "btn_help"), app->help_open, nk_rgba(77, 163, 245, 255))) {
                app->help_open = !app->help_open;
                if (app->help_open) { app->tutorial_open = false; app->recipe_card_open = false; }
            }

            nk_layout_row_push(ctx, mod_w);
            if (fd_ui_button_styled(ctx, T(app, "mode_desktop"), false)) {
                app->touch_mode = true;
                app->touch_manual_override = true;
            }
            nk_layout_row_end(ctx);
        }
    }
    nk_end(ctx);

    float body_y = top_h + 4.0f;
    float body_h = H - body_y - status_h - 4.0f;

    /* Optional sleek non-blocking welcome banner */
    if (app->show_welcome && !app->tutorial_open && !app->help_open && !app->recipe_card_open) {
        float ban_h = app->touch_mode ? 96.0f : 38.0f;
        ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(26, 38, 54, 255));
        if (nk_begin(ctx, "fd_welcome", nk_rect(6.0f, body_y, W - 12.0f, ban_h),
                     NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            if (app->touch_mode) {
                if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
                nk_layout_row_dynamic(ctx, 36.0f, 1);
                nk_label_colored_wrap(ctx, T(app, "welcome_banner"), nk_rgba(218, 232, 250, 255));
                nk_layout_row_dynamic(ctx, 34.0f, 2);
                if (fd_ui_button_pill(ctx, T(app, "btn_start_tour"), true, nk_rgba(52, 199, 133, 255))) {
                    fd_app_start_tutorial(app);
                }
                if (fd_ui_button_styled(ctx, T(app, "btn_dismiss"), false)) {
                    app->show_welcome = false;
                }
                if (app->ui.font_small) nk_style_pop_font(ctx);
            } else {
                nk_layout_row_begin(ctx, NK_DYNAMIC, ban_h - 14.0f, 3);
                nk_layout_row_push(ctx, 0.74f);
                nk_label_colored(ctx, T(app, "welcome_banner"), NK_TEXT_LEFT, nk_rgba(218, 232, 250, 255));
                nk_layout_row_push(ctx, 0.14f);
                if (fd_ui_button_pill(ctx, T(app, "btn_start_tour"), true, nk_rgba(52, 199, 133, 255))) {
                    fd_app_start_tutorial(app);
                }
                nk_layout_row_push(ctx, 0.12f);
                if (fd_ui_button_styled(ctx, T(app, "btn_dismiss"), false)) {
                    app->show_welcome = false;
                }
                nk_layout_row_end(ctx);
            }
        }
        nk_end(ctx);
        ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(22, 28, 39, 255));
        body_y += ban_h + 4.0f;
        body_h -= ban_h + 4.0f;
    }

    /* Mobile Bottom Thumb Navigation Bar (>=48px tap targets) */
    if (app->touch_mode) {
        float nav_h = 58.0f;
        body_h -= nav_h + 4.0f;
        if (nk_begin(ctx, "fd_mobile_nav", nk_rect(0.0f, H - status_h - nav_h, W, nav_h),
                     NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
            nk_layout_row_dynamic(ctx, 48.0f, 4);
            for (int t = 0; t < 4; t++) {
                if (fd_ui_button_styled(ctx, T(app, tab_keys[t]), (int)app->active_tab == t)) {
                    app->active_tab = (fd_tab_t)t;
                }
            }
            if (app->ui.font_small) nk_style_pop_font(ctx);
        }
        nk_end(ctx);
    }

    /* 2. Active Workspace Content */
    if (app->active_tab == FD_TAB_WORKBENCH) {
        if (app->touch_mode) {
            float sub_h = 56.0f;
            if (nk_begin(ctx, "fd_mobile_sub", nk_rect(6.0f, body_y, W - 12.0f, sub_h),
                         NK_WINDOW_NO_SCROLLBAR)) {
                const char *skeys[3] = { "sub_station", "sub_slice", "sub_metrics" };
                nk_layout_row_dynamic(ctx, 48.0f, 3);
                for (int s = 0; s < 3; s++) {
                    if (fd_ui_button_styled(ctx, T(app, skeys[s]), app->mobile_subtab == s)) {
                        app->mobile_subtab = s;
                    }
                }
            }
            nk_end(ctx);
            float cy = body_y + sub_h + 4.0f;
            float ch = fmaxf(120.0f, body_h - sub_h - 4.0f);
            if (app->mobile_subtab == 0)      render_controls_panel(app, 6.0f, cy, W - 12.0f, ch);
            else if (app->mobile_subtab == 1) render_slice_panel(app, 6.0f, cy, W - 12.0f, ch);
            else                              render_telemetry_panel(app, 6.0f, cy, W - 12.0f, ch);
        } else {
            float col_l = clampf(W * 0.30f, 320.0f, 385.0f);
            float col_m = (W - col_l - 24.0f) * 0.48f;
            float col_r = W - col_l - col_m - 24.0f;
            render_controls_panel(app, 6.0f, body_y, col_l, body_h);
            render_slice_panel(app, 12.0f + col_l, body_y, col_m, body_h);
            render_telemetry_panel(app, 18.0f + col_l + col_m, body_y, col_r, body_h);
        }
    } else if (app->active_tab == FD_TAB_PROPOSE) {
        render_propose_tab(app, 6.0f, body_y, W - 12.0f, body_h);
    } else if (app->active_tab == FD_TAB_INVENT) {
        render_invent_tab(app, 6.0f, body_y, W - 12.0f, body_h);
    } else {
        render_mods_tab(app, 6.0f, body_y, W - 12.0f, body_h);
    }

    /* 3. Footer Status Bar */
    if (nk_begin(ctx, "fd_footer", nk_rect(0.0f, H - status_h, W, status_h), NK_WINDOW_NO_SCROLLBAR)) {
        if (app->ui.font_small) nk_style_push_font(ctx, &app->ui.uf_small);
        nk_layout_row_dynamic(ctx, status_h - 10.0f, 1);
        nk_label_colored(ctx, app->status_msg, NK_TEXT_LEFT, nk_rgba(176, 196, 224, 255));
        if (app->ui.font_small) nk_style_pop_font(ctx);
    }
    nk_end(ctx);

    /* 4. Floating Modals & Overlays (drawn last for top Z-order) */
    render_recipe_card_modal(app, W, H);
    render_tutorial_overlay(app, W, H);
    render_help_modal(app, W, H);

    fd_ui_end(&app->ui);

    /* Apply deferred font/locale rebuilds safely after Nuklear command buffer is flushed */
    if (app->pending_reload) {
        app->pending_reload = false;
        if (fd_lua_reload_all(&app->lua)) {
            fd_i18n_set_lang(&app->i18n, &app->lua, app->i18n.active_lang);
            fd_ui_rebuild_fonts(&app->ui, &app->vfs, &app->i18n, app->ui.baked_dpi_scale);
            app->pending_cue = FD_CUE_TIMER;
            set_status(app, "status_reloaded", NULL);
        }
    }
    if (app->pending_lang_idx >= 0) {
        int next_lang = app->pending_lang_idx;
        app->pending_lang_idx = -1;
        fd_app_set_language(app, next_lang);
    }
}
