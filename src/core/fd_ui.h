#ifndef FD_UI_H
#define FD_UI_H

#include "fd_geometry.h"
#include "fd_i18n.h"
#include "fd_nk.h"
#include "fd_sim.h"
#include "fd_vfs.h"

#include <stdbool.h>
#include <stdint.h>

#define FD_MAX_VERTICES 65536
#define FD_MAX_INDICES  131072
#define FD_MAX_CMDS     512

typedef enum {
    FD_TEX_FONT    = 0,
    FD_TEX_KITCHEN = 1,
    FD_TEX_SLICE   = 2,
    FD_TEX_ICONS   = 3,
    FD_TEX_COUNT   = 4
} fd_tex_id_t;

typedef struct {
    float   x, y;
    float   u, v;
    uint8_t r, g, b, a;
} fd_vertex_t;

typedef struct {
    fd_tex_id_t tex_id;
    uint32_t    elem_offset;
    uint32_t    elem_count;
    float       clip_x, clip_y, clip_w, clip_h;
} fd_draw_cmd_t;

typedef struct {
    float width, height;
    float dpi_scale;
    float pointer_x, pointer_y;
    float scroll_y;
    bool  pointer_down;
    bool  pointer_pressed;
    bool  pointer_released;
    bool  is_touch_event;
    bool  key_space;
    bool  key_flip;
    bool  key_reload;
    bool  key_plane;
    bool  key_help;
    int   speed_key; /* 1..5 or 0 */
} fd_input_t;

typedef struct {
    struct nk_context           nk;
    struct nk_font_atlas        atlas;
    struct nk_font             *font_body;
    struct nk_font             *font_title;
    struct nk_font             *font_small;
    struct nk_user_font         uf_body;
    struct nk_user_font         uf_title;
    struct nk_user_font         uf_small;
    struct nk_draw_null_texture null_tex;
    struct nk_buffer            nk_cmds;
    struct nk_buffer            nk_vbuf;
    struct nk_buffer            nk_ebuf;

    /* Baked font atlas RGBA copy for platform GL upload */
    uint8_t *atlas_rgba;
    int      atlas_w;
    int      atlas_h;
    bool     atlas_dirty;
    bool     atlas_ready;
    float    baked_dpi_scale;
    nk_rune  script_ranges[2048]; /* Persistent backing store for nk_font_config.range */

    /* Frame input & touch state */
    fd_input_t in;
    bool       touch_mode;
    bool       initialized;

    /* Extracted renderer-agnostic draw buffers */
    int            vert_count;
    int            index_count;
    int            cmd_count;
    fd_vertex_t   *verts;
    uint16_t      *indices;
    fd_draw_cmd_t *cmds;
} fd_ui_t;

bool fd_ui_init(fd_ui_t *ui, const fd_vfs_t *vfs, const fd_i18n_t *i18n);
void fd_ui_free(fd_ui_t *ui);

/* Rebuild Noto Sans + active language script font atlas into RGBA buffer */
bool fd_ui_rebuild_fonts(fd_ui_t *ui, const fd_vfs_t *vfs, const fd_i18n_t *i18n, float dpi_scale);

/* Frame begin/end converting Nuklear commands into indexed vertex/scissor draw lists */
void fd_ui_begin(fd_ui_t *ui, const fd_input_t *in, bool touch_mode);
void fd_ui_end(fd_ui_t *ui);

/* Modern culinary-lab theme & high-level Nuklear widgets */
void fd_ui_apply_theme(struct nk_context *ctx, bool touch_mode);
bool fd_ui_button_styled(struct nk_context *ctx, const char *label, bool primary_or_active);
bool fd_ui_button_pill(struct nk_context *ctx, const char *label, bool active, struct nk_color accent);
void fd_ui_badge(struct nk_context *ctx, const char *label, struct nk_color bg, struct nk_color fg);
void fd_ui_metric_bar(struct nk_context *ctx, const char *label, const char *val_str,
                      float frac, struct nk_color fill);
void fd_ui_tooltip(struct nk_context *ctx, const char *text, bool touch_mode);
void fd_ui_draw_legend_bar(struct nk_context *ctx, fd_overlay_mode_t mode,
                           const char *cold_lbl, const char *hot_lbl);
void fd_ui_draw_thermal_chart(struct nk_context *ctx, const fd_sim_state_t *sim,
                              const char *lbl_surf, const char *lbl_cold, const char *lbl_mail,
                              float height);
void fd_ui_draw_slice_box(struct nk_context *ctx, const fd_sim_state_t *sim,
                          fd_slice_plane_t plane, float slice_norm, float height);

#endif /* FD_UI_H */
