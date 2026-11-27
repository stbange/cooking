#ifndef FD_APP_H
#define FD_APP_H

#include "fd_geometry.h"
#include "fd_i18n.h"
#include "fd_lua.h"
#include "fd_sim.h"
#include "fd_solver.h"
#include "fd_ui.h"
#include "fd_vfs.h"

typedef enum {
    FD_TAB_WORKBENCH = 0,
    FD_TAB_PROPOSE   = 1,
    FD_TAB_INVENT    = 2,
    FD_TAB_MODS      = 3
} fd_tab_t;

typedef enum {
    FD_CUE_NONE      = 0,
    FD_CUE_SEAR_DROP = 1,
    FD_CUE_CHOP      = 2,
    FD_CUE_TIMER     = 3
} fd_sound_cue_t;

#define FD_TUTORIAL_STEPS 6
#define FD_HELP_TABS      5

typedef struct {
    fd_vfs_t       vfs;
    fd_database_t  db;
    fd_lua_t       lua;
    fd_i18n_t      i18n;
    fd_sim_state_t sim;
    fd_ui_t        ui;

    /* Active UI & touch state */
    fd_tab_t         active_tab;
    int              mobile_subtab; /* 0=Station, 1=Slice, 2=Science */
    bool             touch_mode;
    bool             touch_manual_override;
    bool             paused;
    float            time_scale;    /* 1x .. 120x */

    /* Onboarding Tutorial & Online Help state */
    bool             show_welcome;
    bool             tutorial_open;
    int              tutorial_step; /* 0 .. FD_TUTORIAL_STEPS-1 */
    bool             help_open;
    int              help_tab;      /* 0 .. FD_HELP_TABS-1 */
    bool             lang_changed;
    int              pending_lang_idx; /* -1 when none; applied safely after fd_ui_end */
    bool             pending_reload;
    char             pending_url[160];

    /* 2D Orthographic Slice Inspector state */
    fd_slice_plane_t  slice_plane;
    float             slice_norm;   /* [0..1] depth along normal axis */
    fd_overlay_mode_t overlay_mode;
    uint8_t           slice_rgba[FD_MAX_GRID_DIM * FD_MAX_GRID_DIM * 4];
    int               slice_w, slice_h;
    bool              slice_dirty;

    /* Selected classic recipe & solver state */
    int              selected_recipe_idx;
    fd_constraints_t constraints;
    int              proposal_count;
    fd_proposal_t    proposals[FD_MAX_PROPOSALS];
    fd_inventor_t    inventor;

    /* Mod Lab state */
    bool             mod_boost_enabled;
    char             status_msg[256];

    /* One-shot audio cue emitted during frame */
    fd_sound_cue_t   pending_cue;
} fd_app_t;

bool fd_app_init(fd_app_t *app, const char *explicit_assets_dir, const char *exe_base_path);
bool fd_app_init_with_lang(fd_app_t *app, const char *explicit_assets_dir,
                           const char *exe_base_path, const char *lang_code);
void fd_app_free(fd_app_t *app);

/* Language, Tutorial & Help controls */
bool fd_app_set_language(fd_app_t *app, int lang_idx);
void fd_app_start_tutorial(fd_app_t *app);
void fd_app_tutorial_try_step(fd_app_t *app, int step);
void fd_app_open_help(fd_app_t *app, int tab);

bool fd_app_load_recipe_index(fd_app_t *app, int recipe_idx);
bool fd_app_load_recipe(fd_app_t *app, const fd_recipe_t *r);
void fd_app_snap_cold_spot(fd_app_t *app);
void fd_app_snap_hot_spot(fd_app_t *app);

/* Advance simulation, solver, and Nuklear UI for one frame */
void fd_app_update_and_render(fd_app_t *app, const fd_input_t *in, float frame_dt_sec);

#endif /* FD_APP_H */
