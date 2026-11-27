#include "fd_ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void fd_ui_begin(fd_ui_t *ui, const fd_input_t *in, bool touch_mode) {
    if (!ui || !in) return;
    int prev_active = ui->active_id;
    memset(ui, 0, sizeof(*ui));
    ui->in = *in;
    ui->touch_mode = touch_mode;
    ui->active_id  = (in->pointer_down || in->pointer_released) ? prev_active : 0;
}

static void push_quad(fd_ui_t *ui, fd_tex_id_t tex,
                      float x0, float y0, float x1, float y1,
                      float x2, float y2, float x3, float y3,
                      float u0, float v0, float u1, float v1, fd_color_t c) {
    if (ui->vert_count + 6 > FD_MAX_VERTICES) return;
    if (ui->cmd_count == 0 || ui->cmds[ui->cmd_count - 1].tex_id != tex) {
        if (ui->cmd_count >= FD_MAX_CMDS) return;
        fd_draw_cmd_t *cmd = &ui->cmds[ui->cmd_count++];
        cmd->tex_id = tex;
        cmd->first_vertex = ui->vert_count;
        cmd->vertex_count = 0;
    }
    fd_vertex_t *v = &ui->verts[ui->vert_count];
    v[0] = (fd_vertex_t){ x0, y0, u0, v0, c.r, c.g, c.b, c.a };
    v[1] = (fd_vertex_t){ x1, y1, u1, v0, c.r, c.g, c.b, c.a };
    v[2] = (fd_vertex_t){ x2, y2, u1, v1, c.r, c.g, c.b, c.a };
    v[3] = (fd_vertex_t){ x0, y0, u0, v0, c.r, c.g, c.b, c.a };
    v[4] = (fd_vertex_t){ x2, y2, u1, v1, c.r, c.g, c.b, c.a };
    v[5] = (fd_vertex_t){ x3, y3, u0, v1, c.r, c.g, c.b, c.a };
    ui->vert_count += 6;
    ui->cmds[ui->cmd_count - 1].vertex_count += 6;
}

void fd_ui_rect(fd_ui_t *ui, float x, float y, float w, float h, fd_color_t col) {
    /* (0,0)-(2,2) in 256x128 font atlas is solid white */
    float u = 0.5f / 256.0f, v = 0.5f / 128.0f;
    push_quad(ui, FD_TEX_FONT, x, y, x + w, y, x + w, y + h, x, y + h, u, v, u, v, col);
}

void fd_ui_outline(fd_ui_t *ui, float x, float y, float w, float h, float t, fd_color_t col) {
    fd_ui_rect(ui, x, y, w, t, col);
    fd_ui_rect(ui, x, y + h - t, w, t, col);
    fd_ui_rect(ui, x, y + t, t, h - 2.0f * t, col);
    fd_ui_rect(ui, x + w - t, y + t, t, h - 2.0f * t, col);
}

void fd_ui_line(fd_ui_t *ui, float x0, float y0, float x1, float y1, float thick, fd_color_t col) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    float nx = (-dy / len) * (thick * 0.5f);
    float ny = ( dx / len) * (thick * 0.5f);
    float u = 0.5f / 256.0f, v = 0.5f / 128.0f;
    push_quad(ui, FD_TEX_FONT,
              x0 + nx, y0 + ny, x1 + nx, y1 + ny,
              x1 - nx, y1 - ny, x0 - nx, y0 - ny,
              u, v, u, v, col);
}

void fd_ui_sprite(fd_ui_t *ui, fd_tex_id_t tex, float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1, fd_color_t tint) {
    push_quad(ui, tex, x, y, x + w, y, x + w, y + h, x, y + h, u0, v0, u1, v1, tint);
}

float fd_ui_text_width(const char *str, float scale) {
    if (!str) return 0.0f;
    float w = 0.0f;
    for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
        if (*p == 0xC2 && p[1] == 0xB0) { w += 9.0f * scale; p++; continue; } /* UTF-8 '°' */
        w += 9.0f * scale;
    }
    return w;
}

void fd_ui_text(fd_ui_t *ui, float x, float y, float scale, fd_color_t col, const char *str) {
    if (!ui || !str) return;
    float cx = x;
    float gw = 16.0f * scale, gh = 16.0f * scale;
    for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
        int code = *p;
        if (code == 0xC2 && p[1] == 0xB0) {
            code = 127; /* Mapped degree glyph */
            p++;
        } else if (code < 32 || code > 126) {
            code = 63;
        }
        if (code != 32) {
            int idx = code - 32;
            int gx = (idx % 16) * 16;
            int gy = (idx / 16) * 16 + 16;
            float u0 = (float)gx / 256.0f;
            float v0 = (float)gy / 128.0f;
            float u1 = (float)(gx + 16) / 256.0f;
            float v1 = (float)(gy + 16) / 128.0f;
            push_quad(ui, FD_TEX_FONT, cx, y, cx + gw, y, cx + gw, y + gh, cx, y + gh, u0, v0, u1, v1, col);
        }
        cx += 9.0f * scale;
    }
}

static bool hit_rect(const fd_input_t *in, float x, float y, float w, float h) {
    return in->pointer_x >= x && in->pointer_x <= (x + w) &&
           in->pointer_y >= y && in->pointer_y <= (y + h);
}

bool fd_ui_button(fd_ui_t *ui, int id, float x, float y, float w, float h,
                  const char *label, bool active_highlight) {
    bool hot = hit_rect(&ui->in, x, y, w, h);
    if (hot) ui->hot_id = id;
    if (hot && ui->in.pointer_pressed) ui->active_id = id;

    bool clicked = hot && ui->in.pointer_released && (ui->active_id == id || ui->touch_mode);
    /* Also allow immediate press activation when pointer_pressed + pointer_released occur in one test tick */
    if (hot && ui->in.pointer_pressed && !ui->in.pointer_down) clicked = true;

    fd_color_t bg = active_highlight ? (fd_color_t){ 216, 118, 42, 255 }
                  : (hot ? (fd_color_t){ 56, 66, 84, 255 } : (fd_color_t){ 38, 45, 58, 255 });
    fd_color_t border = active_highlight ? (fd_color_t){ 250, 182, 92, 255 } : (fd_color_t){ 78, 90, 112, 255 };
    fd_ui_rect(ui, x, y, w, h, bg);
    fd_ui_outline(ui, x, y, w, h, 1.5f, border);

    float sc = ui->touch_mode ? 1.05f : 0.95f;
    float tw = fd_ui_text_width(label, sc);
    float tx = x + fmaxf(4.0f, (w - tw) * 0.5f);
    float ty = y + (h - 15.0f * sc) * 0.5f;
    fd_ui_text(ui, tx, ty, sc, (fd_color_t){ 242, 245, 250, 255 }, label);
    return clicked;
}

bool fd_ui_slider(fd_ui_t *ui, int id, float x, float y, float w, float h,
                  const char *label, float *val, float min_v, float max_v, const char *fmt) {
    bool hot = hit_rect(&ui->in, x, y, w, h);
    if (hot && ui->in.pointer_pressed) ui->active_id = id;
    bool changed = false;

    if (ui->in.pointer_down && (ui->active_id == id || (ui->touch_mode && hot))) {
        float t = clampf((ui->in.pointer_x - (x + 6.0f)) / fmaxf(1.0f, w - 12.0f), 0.0f, 1.0f);
        float nv = min_v + t * (max_v - min_v);
        if (fabsf(nv - *val) > 1e-4f) {
            *val = nv;
            changed = true;
        }
    }

    fd_ui_rect(ui, x, y, w, h, (fd_color_t){ 28, 33, 44, 255 });
    float frac = (max_v > min_v) ? clampf((*val - min_v) / (max_v - min_v), 0.0f, 1.0f) : 0.0f;
    fd_ui_rect(ui, x + 2.0f, y + 2.0f, (w - 4.0f) * frac, h - 4.0f, (fd_color_t){ 68, 138, 218, 220 });
    fd_ui_outline(ui, x, y, w, h, 1.0f, (fd_color_t){ 74, 86, 108, 255 });

    char buf[96];
    char vbuf[32];
    snprintf(vbuf, sizeof(vbuf), fmt ? fmt : "%.1f", *val);
    snprintf(buf, sizeof(buf), "%s: %s", label ? label : "", vbuf);
    fd_ui_text(ui, x + 8.0f, y + (h - 15.0f) * 0.5f, 0.92f, (fd_color_t){ 245, 248, 255, 255 }, buf);
    return changed;
}
