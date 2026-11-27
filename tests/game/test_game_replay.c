#include "fd_app.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void render_frame(fd_app_t *app, float w, float h, float dt, bool touch) {
    fd_input_t in;
    memset(&in, 0, sizeof(in));
    in.width          = w;
    in.height         = h;
    in.is_touch_event = touch;
    fd_app_update_and_render(app, &in, dt);
    assert(app->ui.vert_count > 0 && app->ui.index_count > 0 && app->ui.cmd_count > 0);
}

static void click_at(fd_app_t *app, float w, float h, float x, float y, bool touch) {
    fd_input_t in;
    memset(&in, 0, sizeof(in));
    in.width           = w;
    in.height          = h;
    in.pointer_x       = x;
    in.pointer_y       = y;
    in.is_touch_event  = touch;
    in.pointer_down    = true;
    in.pointer_pressed = true;
    fd_app_update_and_render(app, &in, 0.016f);

    in.pointer_down     = false;
    in.pointer_pressed  = false;
    in.pointer_released = true;
    fd_app_update_and_render(app, &in, 0.016f);
}

int main(void) {
    fd_app_t app;
    assert(fd_app_init(&app, "assets", NULL));
    assert(app.db.ingredient_count >= 105);
    assert(app.db.technique_count == 15);
    assert(app.db.recipe_count >= 32);

    const float W = 1240.0f, H = 760.0f;

    /* 1. Desktop Kitchen Lab live frames + welcome banner + hotkeys */
    for (int f = 0; f < 15; f++) render_frame(&app, W, H, 0.05f, false);

    fd_input_t hk;
    memset(&hk, 0, sizeof(hk));
    hk.width = W; hk.height = H;
    hk.key_flip = true; hk.key_plane = true; hk.speed_key = 4; hk.key_help = true;
    fd_app_update_and_render(&app, &hk, 0.05f);
    assert(app.time_scale == 45.0f);
    assert(app.help_open);

    /* Walk through all 5 Help tabs and render cleanly */
    for (int ht = 0; ht < FD_HELP_TABS; ht++) {
        fd_app_open_help(&app, ht);
        render_frame(&app, W, H, 0.016f, false);
    }
    /* Close Help via F1 hotkey */
    memset(&hk, 0, sizeof(hk));
    hk.width = W; hk.height = H; hk.key_help = true; hk.key_space = true; hk.key_reload = true;
    fd_app_update_and_render(&app, &hk, 0.016f);
    assert(!app.help_open);
    assert(app.paused);

    /* Resume */
    memset(&hk, 0, sizeof(hk));
    hk.width = W; hk.height = H; hk.key_space = true;
    fd_app_update_and_render(&app, &hk, 0.016f);
    assert(!app.paused);

    /* 2. Walk through all 6 Interactive Guided Tutorial steps & Try It Now actions */
    fd_app_start_tutorial(&app);
    assert(app.tutorial_open && !app.show_welcome);
    for (int st = 0; st < FD_TUTORIAL_STEPS; st++) {
        app.tutorial_step = st;
        fd_app_tutorial_try_step(&app, st);
        render_frame(&app, W, H, 0.016f, false);
    }
    app.tutorial_open = false;

    /* Let evolutionary inventor triggered in Tutorial step 5 finish cleanly */
    app.active_tab = FD_TAB_INVENT;
    for (int f = 0; f < 12 && !app.inventor.done; f++) {
        render_frame(&app, W, H, 0.016f, false);
    }
    assert(app.inventor.done);
    render_frame(&app, W, H, 0.016f, false);

    /* 3. Exercise Kitchen Lab: recipe load, snap cold/hot spots, all 3 planes & 5 overlays */
    app.active_tab = FD_TAB_WORKBENCH;
    assert(fd_app_load_recipe_index(&app, 1));
    fd_app_snap_cold_spot(&app);
    fd_app_snap_hot_spot(&app);
    for (int pl = 0; pl < 3; pl++) {
        app.slice_plane = (fd_slice_plane_t)pl;
        for (int ov = 0; ov < 5; ov++) {
            app.overlay_mode = (fd_overlay_mode_t)ov;
            render_frame(&app, W, H, 0.02f, false);
        }
    }

    /* 4. Click Desktop header tabs cleanly through Nuklear */
    /* Tab 1 (Classics): logo(34) + title(102) + tab0(108) + spacing -> ~310, y=25 */
    click_at(&app, W, H, 315.0f, 25.0f, false);
    assert(app.active_tab == FD_TAB_PROPOSE);
    render_frame(&app, W, H, 0.016f, false);

    /* Tab 2 (Invent) */
    click_at(&app, W, H, 428.0f, 25.0f, false);
    assert(app.active_tab == FD_TAB_INVENT);
    render_frame(&app, W, H, 0.016f, false);

    /* Tab 3 (Mod Lab) + deferred hot-reload & language switch */
    click_at(&app, W, H, 540.0f, 25.0f, false);
    assert(app.active_tab == FD_TAB_MODS);
    app.pending_reload = true;
    app.pending_lang_idx = fd_i18n_find_lang("es");
    render_frame(&app, W, H, 0.016f, false);
    assert(!app.pending_reload && app.pending_lang_idx == -1);
    assert(app.i18n.active_lang == fd_i18n_find_lang("es"));

    /* Toggle Live Truffle Mod on and off inside Mod Lab */
    click_at(&app, W, H, W * 0.72f, 225.0f, false);
    click_at(&app, W, H, W * 0.72f, 225.0f, false);

    /* 5. Runtime switching across major scripts (EN, DE, ZH, AR RTL, HI Indic, JA, KO, RU) */
    const char *test_langs[] = { "de", "zh", "ar", "hi", "ja", "ko", "ru", "ur", "bn", "te", "ta", "en" };
    for (size_t i = 0; i < sizeof(test_langs) / sizeof(test_langs[0]); i++) {
        assert(fd_app_set_language(&app, fd_i18n_find_lang(test_langs[i])));
        app.active_tab = FD_TAB_WORKBENCH;
        render_frame(&app, W, H, 0.016f, false);
    }

    /* 6. Mobile Touch Device Journey (390x844 iPhone viewport + touch events) */
    const float MW = 390.0f, MH = 844.0f;
    app.touch_manual_override = false;
    app.show_welcome = true;
    /* Tap bottom thumb tab 0 (Kitchen Lab): bottom bar at H-26-58..H-26 -> y ~ 790 */
    click_at(&app, MW, MH, 50.0f, MH - 55.0f, true);
    assert(app.touch_mode);
    assert(app.active_tab == FD_TAB_WORKBENCH);

    /* Cycle mobile subtabs (Station, 3D Slice, Science) + mobile Help + mobile Tutorial */
    for (int sub = 0; sub < 3; sub++) {
        app.mobile_subtab = sub;
        render_frame(&app, MW, MH, 0.016f, true);
    }
    /* Tap bottom thumb tab 2 (Invent) on mobile */
    click_at(&app, MW, MH, 245.0f, MH - 55.0f, true);
    assert(app.active_tab == FD_TAB_INVENT);
    render_frame(&app, MW, MH, 0.016f, true);

    /* Tap bottom thumb tab 3 (Mod Lab) on mobile */
    click_at(&app, MW, MH, 340.0f, MH - 55.0f, true);
    assert(app.active_tab == FD_TAB_MODS);
    render_frame(&app, MW, MH, 0.016f, true);

    /* Mobile Help & Tutorial overlays */
    fd_app_start_tutorial(&app);
    render_frame(&app, MW, MH, 0.016f, true);
    fd_app_open_help(&app, 0);
    assert(!app.tutorial_open && app.help_open);
    render_frame(&app, MW, MH, 0.016f, true);

    fd_app_free(&app);
    printf("ALL AUTOMATED NUKLEAR DESKTOP, MOBILE-TOUCH, TUTORIAL, HELP & I18N REPLAY TESTS PASSED.\n");
    return 0;
}
