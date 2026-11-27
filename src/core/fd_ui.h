#ifndef FD_UI_H
#define FD_UI_H

#include <stdbool.h>
#include <stdint.h>

#define FD_MAX_VERTICES 24576
#define FD_MAX_CMDS     64

typedef enum {
    FD_TEX_FONT    = 0,
    FD_TEX_KITCHEN = 1,
    FD_TEX_SLICE   = 2
} fd_tex_id_t;

typedef struct {
    float x, y;
    float u, v;
    uint8_t r, g, b, a;
} fd_vertex_t;

typedef struct {
    fd_tex_id_t tex_id;
    int         first_vertex;
    int         vertex_count;
} fd_draw_cmd_t;

typedef struct {
    float width, height;
    float pointer_x, pointer_y;
    bool  pointer_down;
    bool  pointer_pressed;
    bool  pointer_released;
    bool  is_touch_event;
    bool  key_space;
    bool  key_flip;
    bool  key_reload;
    bool  key_plane;
    int   speed_key; /* 1..5 or 0 */
} fd_input_t;

typedef struct {
    uint8_t r, g, b, a;
} fd_color_t;

typedef struct {
    fd_input_t    in;
    bool          touch_mode;
    int           active_id;
    int           hot_id;

    int           vert_count;
    fd_vertex_t   verts[FD_MAX_VERTICES];
    int           cmd_count;
    fd_draw_cmd_t cmds[FD_MAX_CMDS];
} fd_ui_t;

void fd_ui_begin(fd_ui_t *ui, const fd_input_t *in, bool touch_mode);

/* Low-level batched primitives */
void fd_ui_rect(fd_ui_t *ui, float x, float y, float w, float h, fd_color_t col);
void fd_ui_outline(fd_ui_t *ui, float x, float y, float w, float h, float thick, fd_color_t col);
void fd_ui_line(fd_ui_t *ui, float x0, float y0, float x1, float y1, float thick, fd_color_t col);
void fd_ui_sprite(fd_ui_t *ui, fd_tex_id_t tex, float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1, fd_color_t tint);
void fd_ui_text(fd_ui_t *ui, float x, float y, float scale, fd_color_t col, const char *str);
float fd_ui_text_width(const char *str, float scale);

/* Immediate-mode interactive controls */
bool fd_ui_button(fd_ui_t *ui, int id, float x, float y, float w, float h,
                  const char *label, bool active_highlight);
bool fd_ui_slider(fd_ui_t *ui, int id, float x, float y, float w, float h,
                  const char *label, float *val, float min_v, float max_v, const char *fmt);

#endif /* FD_UI_H */
