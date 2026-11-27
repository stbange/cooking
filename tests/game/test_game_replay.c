#include "fd_app.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

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

static void drag_slider(fd_app_t *app, float w, float h, float x0, float x1, float y, bool touch) {
    fd_input_t in;
    memset(&in, 0, sizeof(in));
    in.width = w; in.height = h;
    in.is_touch_event = touch;
    in.pointer_x = x0; in.pointer_y = y;
    in.pointer_down = true; in.pointer_pressed = true;
    fd_app_update_and_render(app, &in, 0.016f);

    in.pointer_pressed = false;
    in.pointer_x = x1;
    fd_app_update_and_render(app, &in, 0.016f);

    in.pointer_down = false; in.pointer_released = true;
    fd_app_update_and_render(app, &in, 0.016f);
}

int main(void) {
    fd_app_t app;
    assert(fd_app_init(&app, "assets", NULL));
    assert(app.db.ingredient_count >= 105);
    assert(app.db.technique_count == 15);
    assert(app.db.recipe_count >= 32);

    const float W = 1180.0f, H = 720.0f;
    fd_input_t idle = { .width = W, .height = H };

    /* 1. Desktop Workbench simulation frames & hotkeys */
    for (int f = 0; f < 30; f++) {
        fd_app_update_and_render(&app, &idle, 0.05f);
        assert(app.ui.vert_count > 0 && app.ui.cmd_count > 0);
    }

    /* Hotkeys: Space, Flip, Plane cycle, Speed 1..5, F5 reload */
    fd_input_t hk = idle;
    hk.key_flip = true; hk.key_plane = true; hk.speed_key = 4;
    fd_app_update_and_render(&app, &hk, 0.05f);
    assert(app.time_scale == 45.0f);

    hk = idle; hk.key_space = true; hk.key_reload = true;
    fd_app_update_and_render(&app, &hk, 0.05f);
    assert(app.paused);
    hk = idle; hk.key_space = true;
    fd_app_update_and_render(&app, &hk, 0.05f);
    assert(!app.paused);

    /* Click Next Dish, Technique cycle, Setpoint slider, Lid, Flip, Auto-Complete */
    click_at(&app, W, H, 280.0f, 68.0f, false);  /* Next Dish */
    click_at(&app, W, H, 180.0f, 130.0f, false); /* Cycle Technique */
    drag_slider(&app, W, H, 60.0f, 240.0f, 195.0f, false); /* Setpoint slider */
    click_at(&app, W, H, 180.0f, 275.0f, false); /* Flip / Toss 3D */
    click_at(&app, W, H, 280.0f, 275.0f, false); /* Lid toggle */
    click_at(&app, W, H, 180.0f, 315.0f, false); /* Auto-complete classic recipe */

    /* Slice panel buttons: XY, XZ, YZ, ColdSpot, PeakChar, Overlays, Slice Depth */
    float mid_x = 12.0f + 340.0f;
    click_at(&app, W, H, mid_x + 25.0f,  64.0f, false); /* XY */
    click_at(&app, W, H, mid_x + 110.0f, 64.0f, false); /* XZ */
    click_at(&app, W, H, mid_x + 190.0f, 64.0f, false); /* YZ */
    click_at(&app, W, H, mid_x + 265.0f, 64.0f, false); /* ColdSpot */
    click_at(&app, W, H, mid_x + 345.0f, 64.0f, false); /* PeakChar */
    for (int ov = 0; ov < 5; ov++) {
        click_at(&app, W, H, mid_x + 25.0f + (float)ov * 78.0f, 100.0f, false);
    }
    drag_slider(&app, W, H, mid_x + 40.0f, mid_x + 260.0f, 134.0f, false);

    /* 2. Switch to Propose tab (top tab #2) */
    click_at(&app, W, H, 255.0f + 118.0f + 40.0f, 20.0f, false);
    assert(app.active_tab == FD_TAB_PROPOSE);
    click_at(&app, W, H, 80.0f, 100.0f, false);  /* Toggle Vegan */
    click_at(&app, W, H, 980.0f, 100.0f, false); /* Toggle Low-GI */
    click_at(&app, W, H, 820.0f, 140.0f, false); /* Match & Verify Classics */
    assert(app.proposal_count > 0);
    click_at(&app, W, H, W - 90.0f, 190.0f, false); /* Cook #1 proposed dish */
    assert(app.active_tab == FD_TAB_WORKBENCH);

    /* 3. Switch to Invent tab (top tab #3) */
    click_at(&app, W, H, 255.0f + 2.0f * 118.0f + 40.0f, 20.0f, false);
    assert(app.active_tab == FD_TAB_INVENT);
    click_at(&app, W, H, 980.0f, 140.0f, false); /* Click Invent Recipe! */
    assert(app.inventor.active);
    for (int f = 0; f < 10 && !app.inventor.done; f++) {
        fd_app_update_and_render(&app, &idle, 0.016f);
    }
    assert(app.inventor.done);
    click_at(&app, W, H, 120.0f, H - 50.0f, false); /* Load invented dish into Workbench */
    assert(app.active_tab == FD_TAB_WORKBENCH);

    /* 4. Switch to Mods tab (top tab #4) & test live VFS Lua mod injection */
    click_at(&app, W, H, 255.0f + 3.0f * 118.0f + 40.0f, 20.0f, false);
    assert(app.active_tab == FD_TAB_MODS);
    int base_ings = app.db.ingredient_count;
    click_at(&app, W, H, 420.0f, 135.0f, false); /* Inject Live Lua Truffle Mod Override */
    assert(app.db.ingredient_count == base_ings + 1);
    click_at(&app, W, H, 120.0f, 135.0f, false); /* Hot-reload button */
    click_at(&app, W, H, 420.0f, 135.0f, false); /* Disable override */
    assert(app.db.ingredient_count == base_ings);

    /* 5. Mobile Touch Device Journey (390x844 iPhone viewport + touch events) */
    const float MW = 390.0f, MH = 844.0f;
    click_at(&app, MW, MH, 48.0f, MH - 52.0f, true); /* Bottom thumb tab 0: Workbench */
    assert(app.touch_mode);
    assert(app.active_tab == FD_TAB_WORKBENCH);

    /* Tap mobile sub-tabs: 1. Station, 2. 3D Slice, 3. Metrics */
    click_at(&app, MW, MH, 60.0f,  80.0f, true);
    assert(app.mobile_subtab == 0);
    click_at(&app, MW, MH, 195.0f, 80.0f, true);
    assert(app.mobile_subtab == 1);
    click_at(&app, MW, MH, 330.0f, 80.0f, true);
    assert(app.mobile_subtab == 2);

    fd_app_free(&app);
    printf("ALL AUTOMATED HEADLESS DESKTOP & MOBILE-TOUCH GAME REPLAY TESTS PASSED.\n");
    return 0;
}
