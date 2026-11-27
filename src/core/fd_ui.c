#include "fd_ui.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static const nk_rune g_base_ranges[] = {
    0x0020, 0x007E, /* Basic ASCII */
    0x00A0, 0x017F, /* Latin-1 + Latin Extended-A */
    0x0400, 0x045F, /* Cyrillic (Russian) */
    0x1E00, 0x1EFF, /* Vietnamese Latin Extended */
    0x2013, 0x2026, /* En/em dash, quotes, bullet, ellipsis */
    0x2190, 0x2193, /* Arrows */
    0x2260, 0x2265, /* Math comparisons */
    0
};

static const nk_rune g_title_base_ranges[] = {
    0x0020, 0x007E,
    0x00A0, 0x017F,
    0x0400, 0x045F,
    0x1E00, 0x1EFF,
    0x2013, 0x2026,
    0
};

bool fd_ui_rebuild_fonts(fd_ui_t *ui, const fd_vfs_t *vfs, const fd_i18n_t *i18n, float dpi_scale) {
    if (!ui) return false;
    dpi_scale = clampf(dpi_scale > 0.0f ? dpi_scale : 1.0f, 1.0f, 2.0f);

    if (ui->atlas_ready) {
        nk_font_atlas_clear(&ui->atlas);
        ui->atlas_ready = false;
    }

    uint8_t *ui_ttf = NULL, *bold_ttf = NULL, *script_ttf = NULL;
    size_t ui_sz = 0, bold_sz = 0, script_sz = 0;

    if (vfs) {
        fd_vfs_read(vfs, "fonts/NotoSans-UI.ttf", &ui_ttf, &ui_sz);
        fd_vfs_read(vfs, "fonts/NotoSans-Bold.ttf", &bold_ttf, &bold_sz);
        const fd_lang_info_t *lang = i18n ? fd_i18n_lang_info(i18n->active_lang) : NULL;
        if (lang && lang->font_path) {
            fd_vfs_read(vfs, lang->font_path, &script_ttf, &script_sz);
        }
    }

    int range_pairs = 0;
    ui->script_ranges[0] = 0;
    if (i18n && script_ttf) {
        range_pairs = fd_i18n_build_rune_ranges(i18n, ui->script_ranges, 2048);
    }

    nk_font_atlas_init_default(&ui->atlas);
    nk_font_atlas_begin(&ui->atlas);

    float body_px  = (script_ttf != NULL) ? 18.0f : 16.0f;
    float title_px = 19.5f;
    float small_px = 13.5f;
    unsigned char ov_h = 2;
    unsigned char ov_v = (script_ttf != NULL) ? 1 : 2;

    if (ui_ttf && ui_sz > 0) {
        struct nk_font_config bcfg = nk_font_config(body_px);
        bcfg.range        = g_base_ranges;
        bcfg.oversample_h = ov_h;
        bcfg.oversample_v = ov_v;
        bcfg.pixel_snap   = nk_false;
        ui->font_body = nk_font_atlas_add_from_memory(&ui->atlas, ui_ttf, (nk_size)ui_sz, body_px, &bcfg);
        if (script_ttf && script_sz > 0 && range_pairs > 0) {
            /* Nuklear merge_mode always merges onto atlas->fonts[0], so derive title/small handles from font_body */
            struct nk_font_config scfg = nk_font_config(body_px);
            scfg.merge_mode   = nk_true;
            scfg.range        = ui->script_ranges;
            scfg.oversample_h = 1;
            scfg.oversample_v = 1;
            nk_font_atlas_add_from_memory(&ui->atlas, script_ttf, (nk_size)script_sz, body_px, &scfg);
            ui->font_title = ui->font_body;
            ui->font_small = ui->font_body;
        } else {
            uint8_t *tblob = (bold_ttf && bold_sz > 0) ? bold_ttf : ui_ttf;
            size_t   tsz   = (bold_ttf && bold_sz > 0) ? bold_sz  : ui_sz;
            struct nk_font_config tcfg = nk_font_config(title_px);
            tcfg.range        = g_title_base_ranges;
            tcfg.oversample_h = 2;
            tcfg.oversample_v = 1;
            ui->font_title = nk_font_atlas_add_from_memory(&ui->atlas, tblob, (nk_size)tsz, title_px, &tcfg);

            struct nk_font_config mcfg = nk_font_config(small_px);
            mcfg.range        = g_title_base_ranges;
            mcfg.oversample_h = 1;
            mcfg.oversample_v = 1;
            ui->font_small = nk_font_atlas_add_from_memory(&ui->atlas, ui_ttf, (nk_size)ui_sz, small_px, &mcfg);
        }
    } else {
        ui->font_body  = nk_font_atlas_add_default(&ui->atlas, 15.0f, NULL);
        ui->font_title = ui->font_body;
        ui->font_small = ui->font_body;
    }

    int aw = 0, ah = 0;
    const void *img = nk_font_atlas_bake(&ui->atlas, &aw, &ah, NK_FONT_ATLAS_RGBA32);
    if (img && aw > 0 && ah > 0) {
        size_t bytes = (size_t)aw * (size_t)ah * 4u;
        uint8_t *copy = (uint8_t *)realloc(ui->atlas_rgba, bytes);
        if (copy) {
            memcpy(copy, img, bytes);
            ui->atlas_rgba  = copy;
            ui->atlas_w     = aw;
            ui->atlas_h     = ah;
            ui->atlas_dirty = true;
        }
    }

    nk_font_atlas_end(&ui->atlas, nk_handle_id((int)FD_TEX_FONT), &ui->null_tex);
    ui->atlas_ready     = true;
    ui->baked_dpi_scale = dpi_scale;

    if (ui->font_body) {
        ui->uf_body  = ui->font_body->handle;
        ui->uf_title = ui->font_title ? ui->font_title->handle : ui->font_body->handle;
        ui->uf_small = ui->font_small ? ui->font_small->handle : ui->font_body->handle;
        if (script_ttf && range_pairs > 0) {
            ui->uf_body.height  = 15.5f;
            ui->uf_title.height = 19.0f;
            ui->uf_small.height = 13.5f;
        }
    }

    free(ui_ttf);
    free(bold_ttf);
    free(script_ttf);

    if (ui->initialized && ui->font_body) {
        nk_style_set_font(&ui->nk, &ui->uf_body);
    }
    return ui->font_body != NULL;
}

bool fd_ui_init(fd_ui_t *ui, const fd_vfs_t *vfs, const fd_i18n_t *i18n) {
    if (!ui) return false;
    memset(ui, 0, sizeof(*ui));

    ui->verts   = (fd_vertex_t *)calloc(FD_MAX_VERTICES, sizeof(fd_vertex_t));
    ui->indices = (uint16_t *)calloc(FD_MAX_INDICES, sizeof(uint16_t));
    ui->cmds    = (fd_draw_cmd_t *)calloc(FD_MAX_CMDS, sizeof(fd_draw_cmd_t));
    if (!ui->verts || !ui->indices || !ui->cmds) {
        fd_ui_free(ui);
        return false;
    }

    if (!fd_ui_rebuild_fonts(ui, vfs, i18n, 1.0f)) {
        fd_ui_free(ui);
        return false;
    }

    nk_init_default(&ui->nk, &ui->uf_body);
    nk_buffer_init_default(&ui->nk_cmds);
    fd_ui_apply_theme(&ui->nk, false);
    ui->initialized = true;
    return true;
}

void fd_ui_free(fd_ui_t *ui) {
    if (!ui) return;
    if (ui->initialized) {
        nk_buffer_free(&ui->nk_cmds);
        nk_free(&ui->nk);
        ui->initialized = false;
    }
    if (ui->atlas_ready) {
        nk_font_atlas_clear(&ui->atlas);
        ui->atlas_ready = false;
    }
    free(ui->atlas_rgba);
    free(ui->verts);
    free(ui->indices);
    free(ui->cmds);
    memset(ui, 0, sizeof(*ui));
}

void fd_ui_apply_theme(struct nk_context *ctx, bool touch_mode) {
    if (!ctx) return;
    struct nk_color tbl[NK_COLOR_COUNT];
    tbl[NK_COLOR_TEXT]                    = nk_rgba(232, 238, 248, 255);
    tbl[NK_COLOR_WINDOW]                  = nk_rgba(24, 30, 42, 255);
    tbl[NK_COLOR_HEADER]                  = nk_rgba(32, 40, 56, 255);
    tbl[NK_COLOR_BORDER]                  = nk_rgba(50, 62, 84, 255);
    tbl[NK_COLOR_BUTTON]                  = nk_rgba(37, 47, 66, 255);
    tbl[NK_COLOR_BUTTON_HOVER]            = nk_rgba(52, 66, 92, 255);
    tbl[NK_COLOR_BUTTON_ACTIVE]           = nk_rgba(235, 166, 52, 255);
    tbl[NK_COLOR_TOGGLE]                  = nk_rgba(44, 55, 76, 255);
    tbl[NK_COLOR_TOGGLE_HOVER]            = nk_rgba(58, 74, 102, 255);
    tbl[NK_COLOR_TOGGLE_CURSOR]           = nk_rgba(235, 166, 52, 255);
    tbl[NK_COLOR_SELECT]                  = nk_rgba(32, 40, 56, 255);
    tbl[NK_COLOR_SELECT_ACTIVE]           = nk_rgba(235, 166, 52, 255);
    tbl[NK_COLOR_SLIDER]                  = nk_rgba(36, 46, 64, 255);
    tbl[NK_COLOR_SLIDER_CURSOR]           = nk_rgba(235, 166, 52, 255);
    tbl[NK_COLOR_SLIDER_CURSOR_HOVER]     = nk_rgba(250, 188, 76, 255);
    tbl[NK_COLOR_SLIDER_CURSOR_ACTIVE]    = nk_rgba(255, 206, 104, 255);
    tbl[NK_COLOR_PROPERTY]                = nk_rgba(34, 43, 60, 255);
    tbl[NK_COLOR_EDIT]                    = nk_rgba(28, 36, 50, 255);
    tbl[NK_COLOR_EDIT_CURSOR]             = nk_rgba(235, 166, 52, 255);
    tbl[NK_COLOR_COMBO]                   = nk_rgba(37, 47, 66, 255);
    tbl[NK_COLOR_CHART]                   = nk_rgba(18, 22, 32, 255);
    tbl[NK_COLOR_CHART_COLOR]             = nk_rgba(235, 166, 52, 255);
    tbl[NK_COLOR_CHART_COLOR_HIGHLIGHT]   = nk_rgba(255, 210, 102, 255);
    tbl[NK_COLOR_SCROLLBAR]               = nk_rgba(22, 28, 40, 255);
    tbl[NK_COLOR_SCROLLBAR_CURSOR]        = nk_rgba(68, 84, 114, 255);
    tbl[NK_COLOR_SCROLLBAR_CURSOR_HOVER]  = nk_rgba(92, 114, 152, 255);
    tbl[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(235, 166, 52, 255);
    tbl[NK_COLOR_TAB_HEADER]              = nk_rgba(32, 40, 56, 255);
    tbl[NK_COLOR_KNOB]                    = tbl[NK_COLOR_SLIDER];
    tbl[NK_COLOR_KNOB_CURSOR]             = tbl[NK_COLOR_SLIDER_CURSOR];
    tbl[NK_COLOR_KNOB_CURSOR_HOVER]       = tbl[NK_COLOR_SLIDER_CURSOR_HOVER];
    tbl[NK_COLOR_KNOB_CURSOR_ACTIVE]      = tbl[NK_COLOR_SLIDER_CURSOR_ACTIVE];
    nk_style_from_table(ctx, tbl);

    /* Modern rounded geometry & generous breathing room */
    ctx->style.window.rounding         = 10.0f;
    ctx->style.window.border           = 1.0f;
    ctx->style.window.padding          = touch_mode ? nk_vec2(10.0f, 10.0f) : nk_vec2(10.0f, 8.0f);
    ctx->style.window.spacing          = touch_mode ? nk_vec2(8.0f, 8.0f)   : nk_vec2(6.0f, 6.0f);
    ctx->style.window.scrollbar_size   = touch_mode ? nk_vec2(14.0f, 14.0f) : nk_vec2(10.0f, 10.0f);
    ctx->style.window.group_border     = 1.0f;
    ctx->style.window.group_padding    = nk_vec2(8.0f, 8.0f);
    ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(22, 28, 39, 255));

    ctx->style.button.rounding         = 7.0f;
    ctx->style.button.border           = 1.0f;
    ctx->style.button.border_color     = nk_rgba(62, 78, 104, 255);
    ctx->style.button.padding          = touch_mode ? nk_vec2(8.0f, 8.0f) : nk_vec2(6.0f, 4.0f);

    ctx->style.combo.rounding          = 7.0f;
    ctx->style.combo.border            = 1.0f;
    ctx->style.combo.border_color      = nk_rgba(68, 84, 114, 255);

    ctx->style.slider.bar_height       = touch_mode ? 10.0f : 6.0f;
    ctx->style.slider.cursor_size      = touch_mode ? nk_vec2(24.0f, 24.0f) : nk_vec2(16.0f, 16.0f);
    ctx->style.slider.rounding         = 4.0f;
    ctx->style.slider.bar_filled       = nk_rgba(235, 166, 52, 255);

    ctx->style.progress.rounding       = 5.0f;
    ctx->style.progress.cursor_normal  = nk_style_item_color(nk_rgba(52, 199, 133, 255));
    ctx->style.progress.normal         = nk_style_item_color(nk_rgba(30, 38, 54, 255));
}

void fd_ui_begin(fd_ui_t *ui, const fd_input_t *in, bool touch_mode) {
    if (!ui || !in || !ui->initialized) return;
    ui->in = *in;
    if (ui->touch_mode != touch_mode) {
        ui->touch_mode = touch_mode;
        fd_ui_apply_theme(&ui->nk, touch_mode);
    }

    int px = (int)roundf(in->pointer_x);
    int py = (int)roundf(in->pointer_y);

    nk_input_begin(&ui->nk);
    nk_input_motion(&ui->nk, px, py);
    if (in->pointer_pressed) {
        nk_input_button(&ui->nk, NK_BUTTON_LEFT, px, py, nk_true);
    } else if (in->pointer_released) {
        nk_input_button(&ui->nk, NK_BUTTON_LEFT, px, py, nk_false);
    } else {
        nk_input_button(&ui->nk, NK_BUTTON_LEFT, px, py, in->pointer_down ? nk_true : nk_false);
    }
    if (fabsf(in->scroll_y) > 0.001f) {
        nk_input_scroll(&ui->nk, nk_vec2(0.0f, in->scroll_y));
    }
    nk_input_end(&ui->nk);
}

void fd_ui_end(fd_ui_t *ui) {
    if (!ui || !ui->initialized) return;

    static const struct nk_draw_vertex_layout_element vlayout[] = {
        { NK_VERTEX_POSITION, NK_FORMAT_FLOAT,    offsetof(fd_vertex_t, x) },
        { NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT,    offsetof(fd_vertex_t, u) },
        { NK_VERTEX_COLOR,    NK_FORMAT_R8G8B8A8, offsetof(fd_vertex_t, r) },
        { NK_VERTEX_LAYOUT_END }
    };

    struct nk_convert_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.vertex_layout        = vlayout;
    cfg.vertex_size          = sizeof(fd_vertex_t);
    cfg.vertex_alignment     = NK_ALIGNOF(fd_vertex_t);
    cfg.tex_null             = ui->null_tex;
    cfg.circle_segment_count = 20;
    cfg.curve_segment_count  = 20;
    cfg.arc_segment_count    = 20;
    cfg.global_alpha         = 1.0f;
    cfg.shape_AA             = NK_ANTI_ALIASING_ON;
    cfg.line_AA              = NK_ANTI_ALIASING_ON;

    nk_buffer_clear(&ui->nk_cmds);
    nk_buffer_init_fixed(&ui->nk_vbuf, ui->verts, sizeof(fd_vertex_t) * FD_MAX_VERTICES);
    nk_buffer_init_fixed(&ui->nk_ebuf, ui->indices, sizeof(uint16_t) * FD_MAX_INDICES);
    nk_convert(&ui->nk, &ui->nk_cmds, &ui->nk_vbuf, &ui->nk_ebuf, &cfg);

    size_t vbytes = ui->nk_vbuf.needed;
    size_t ebytes = ui->nk_ebuf.needed;
    ui->vert_count  = (int)(vbytes / sizeof(fd_vertex_t));
    ui->index_count = (int)(ebytes / sizeof(uint16_t));
    if (ui->vert_count > FD_MAX_VERTICES) ui->vert_count = FD_MAX_VERTICES;
    if (ui->index_count > FD_MAX_INDICES) ui->index_count = FD_MAX_INDICES;

    ui->cmd_count = 0;
    uint32_t offset = 0;
    const struct nk_draw_command *cmd = NULL;
    nk_draw_foreach(cmd, &ui->nk, &ui->nk_cmds) {
        if (!cmd->elem_count) continue;
        if (ui->cmd_count < FD_MAX_CMDS) {
            fd_draw_cmd_t *out = &ui->cmds[ui->cmd_count++];
            int tid = cmd->texture.id;
            out->tex_id      = (tid >= 0 && tid < FD_TEX_COUNT) ? (fd_tex_id_t)tid : FD_TEX_FONT;
            out->elem_offset = offset;
            out->elem_count  = cmd->elem_count;
            out->clip_x      = cmd->clip_rect.x;
            out->clip_y      = cmd->clip_rect.y;
            out->clip_w      = cmd->clip_rect.w;
            out->clip_h      = cmd->clip_rect.h;
        }
        offset += cmd->elem_count;
    }

    nk_clear(&ui->nk);
}

bool fd_ui_button_pill(struct nk_context *ctx, const char *label, bool active, struct nk_color accent) {
    struct nk_style_button st = ctx->style.button;
    if (active) {
        st.normal       = nk_style_item_color(accent);
        st.hover        = nk_style_item_color(nk_rgba(
                              (int)fminf(255.0f, (float)accent.r + 18.0f),
                              (int)fminf(255.0f, (float)accent.g + 18.0f),
                              (int)fminf(255.0f, (float)accent.b + 18.0f), 255));
        st.active       = nk_style_item_color(accent);
        st.border_color = accent;
        st.text_normal  = nk_rgba(16, 20, 28, 255);
        st.text_hover   = nk_rgba(16, 20, 28, 255);
        st.text_active  = nk_rgba(16, 20, 28, 255);
    }
    return nk_button_label_styled(ctx, &st, label) != 0;
}

bool fd_ui_button_styled(struct nk_context *ctx, const char *label, bool primary_or_active) {
    return fd_ui_button_pill(ctx, label, primary_or_active, nk_rgba(235, 166, 52, 255));
}

void fd_ui_badge(struct nk_context *ctx, const char *label, struct nk_color bg, struct nk_color fg) {
    struct nk_rect b;
    if (!nk_widget(&b, ctx)) return;
    struct nk_command_buffer *out = nk_window_get_canvas(ctx);
    nk_fill_rect(out, b, 8.0f, bg);
    int len = (int)strlen(label);
    float tw = ctx->style.font->width(ctx->style.font->userdata, ctx->style.font->height, label, len);
    float tx = b.x + fmaxf(6.0f, (b.w - tw) * 0.5f);
    float ty = b.y + fmaxf(1.0f, (b.h - ctx->style.font->height) * 0.5f);
    nk_draw_text(out, nk_rect(tx, ty, fmaxf(4.0f, b.w - 8.0f), ctx->style.font->height + 2.0f),
                 label, len, ctx->style.font, bg, fg);
}

void fd_ui_metric_bar(struct nk_context *ctx, const char *label, const char *val_str,
                      float frac, struct nk_color fill) {
    struct nk_rect b;
    if (!nk_widget(&b, ctx)) return;
    struct nk_command_buffer *out = nk_window_get_canvas(ctx);
    frac = clampf(frac, 0.0f, 1.0f);

    float bar_h = 6.0f;
    struct nk_rect text_r = nk_rect(b.x, b.y, b.w, b.h - bar_h - 3.0f);
    nk_draw_text(out, text_r, label, (int)strlen(label), ctx->style.font,
                 nk_rgba(0, 0, 0, 0), nk_rgba(196, 208, 228, 255));

    float vw = ctx->style.font->width(ctx->style.font->userdata, ctx->style.font->height,
                                      val_str, (int)strlen(val_str));
    struct nk_rect val_r = nk_rect(b.x + b.w - vw - 2.0f, b.y, vw + 4.0f, text_r.h);
    nk_draw_text(out, val_r, val_str, (int)strlen(val_str), ctx->style.font,
                 nk_rgba(0, 0, 0, 0), nk_rgba(238, 244, 252, 255));

    struct nk_rect track = nk_rect(b.x, b.y + b.h - bar_h, b.w, bar_h);
    nk_fill_rect(out, track, 3.0f, nk_rgba(32, 40, 56, 255));
    if (frac > 0.005f) {
        float bw = fmaxf(3.0f, b.w * frac);
        float rr = fminf(3.0f, bw * 0.45f);
        struct nk_rect bar = nk_rect(b.x, b.y + b.h - bar_h, bw, bar_h);
        nk_fill_rect(out, bar, rr, fill);
    }
}

void fd_ui_tooltip(struct nk_context *ctx, const char *text, bool touch_mode) {
    if (!ctx || !text || !text[0] || touch_mode) return;
    if (nk_widget_is_hovered(ctx)) {
        nk_tooltip(ctx, text);
    }
}

static struct nk_color legend_stop(fd_overlay_mode_t mode, float t) {
    t = clampf(t, 0.0f, 1.0f);
    if (mode == FD_OVERLAY_TEMP) {
        if (t < 0.35f) {
            float k = t / 0.35f;
            return nk_rgba((int)(34 + 34 * k), (int)(92 + 128 * k), (int)(225 - 45 * k), 255);
        } else if (t < 0.70f) {
            float k = (t - 0.35f) / 0.35f;
            return nk_rgba((int)(68 + 180 * k), (int)(220 - 30 * k), (int)(180 - 140 * k), 255);
        }
        float k = (t - 0.70f) / 0.30f;
        return nk_rgba((int)(248 + 7 * k), (int)(190 - 140 * k), (int)(40 - 16 * k), 255);
    }
    if (mode == FD_OVERLAY_MOISTURE) {
        return nk_rgba((int)(196 - 156 * t), (int)(142 + 44 * t), (int)(74 + 174 * t), 255);
    }
    if (mode == FD_OVERLAY_MAILLARD) {
        return nk_rgba((int)(238 - 190 * t), (int)(206 - 172 * t), (int)(152 - 126 * t), 255);
    }
    if (mode == FD_OVERLAY_PROTEIN) {
        return nk_rgba((int)(218 - 110 * t), (int)(94 + 122 * t), (int)(104 + 114 * t), 255);
    }
    /* FD_OVERLAY_LETHALITY */
    return nk_rgba((int)(226 - 178 * t), (int)(78 + 132 * t), (int)(72 + 54 * t), 255);
}

void fd_ui_draw_legend_bar(struct nk_context *ctx, fd_overlay_mode_t mode,
                           const char *cold_lbl, const char *hot_lbl) {
    struct nk_rect b;
    if (!nk_widget(&b, ctx)) return;
    struct nk_command_buffer *out = nk_window_get_canvas(ctx);
    const int segs = 24;
    float bar_h = 10.0f;
    float sw = b.w / (float)segs;
    for (int i = 0; i < segs; i++) {
        float t = ((float)i + 0.5f) / (float)segs;
        nk_fill_rect(out, nk_rect(b.x + (float)i * sw, b.y, sw + 0.8f, bar_h), 0.0f, legend_stop(mode, t));
    }
    nk_stroke_rect(out, nk_rect(b.x, b.y, b.w, bar_h), 2.0f, 1.0f, nk_rgba(68, 84, 114, 255));

    const char *lo = "0%", *mid = "50%", *hi = "100%";
    if (mode == FD_OVERLAY_TEMP)           { lo = "20°C"; mid = "120°C"; hi = "240°C"; }
    else if (mode == FD_OVERLAY_LETHALITY) { lo = "0D";   mid = "3.5D";  hi = ">=6.5D"; }

    float ty = b.y + bar_h + 2.0f;
    float th = fmaxf(12.0f, b.h - bar_h - 2.0f);
    struct nk_color tc = nk_rgba(168, 184, 210, 255);
    nk_draw_text(out, nk_rect(b.x, ty, 70.0f, th), lo, (int)strlen(lo), ctx->style.font, nk_rgba(0,0,0,0), tc);
    nk_draw_text(out, nk_rect(b.x + b.w * 0.45f, ty, 70.0f, th), mid, (int)strlen(mid), ctx->style.font, nk_rgba(0,0,0,0), tc);
    float hw = ctx->style.font->width(ctx->style.font->userdata, ctx->style.font->height, hi, (int)strlen(hi));
    nk_draw_text(out, nk_rect(b.x + b.w - hw - 2.0f, ty, hw + 4.0f, th), hi, (int)strlen(hi), ctx->style.font, nk_rgba(0,0,0,0), tc);

    /* Cold-spot & peak-crust marker key if tall enough */
    if (b.h >= 40.0f && cold_lbl && hot_lbl) {
        float ky = ty + 16.0f;
        nk_stroke_circle(out, nk_rect(b.x + 2.0f, ky + 2.0f, 10.0f, 10.0f), 2.0f, nk_rgba(77, 163, 245, 255));
        nk_draw_text(out, nk_rect(b.x + 16.0f, ky, b.w * 0.48f, 16.0f), cold_lbl, (int)strlen(cold_lbl),
                     ctx->style.font, nk_rgba(0,0,0,0), nk_rgba(164, 206, 250, 255));
        float hx = b.x + b.w * 0.52f;
        nk_stroke_circle(out, nk_rect(hx, ky + 2.0f, 10.0f, 10.0f), 2.0f, nk_rgba(245, 166, 52, 255));
        nk_draw_text(out, nk_rect(hx + 14.0f, ky, b.w * 0.46f, 16.0f), hot_lbl, (int)strlen(hot_lbl),
                     ctx->style.font, nk_rgba(0,0,0,0), nk_rgba(250, 204, 122, 255));
    }
}

void fd_ui_draw_slice_box(struct nk_context *ctx, const fd_sim_state_t *sim,
                          fd_slice_plane_t plane, float slice_norm, float height) {
    nk_layout_row_dynamic(ctx, height, 1);
    struct nk_rect b;
    if (!nk_widget(&b, ctx)) return;
    struct nk_command_buffer *out = nk_window_get_canvas(ctx);

    nk_fill_rect(out, b, 8.0f, nk_rgba(14, 18, 26, 255));
    float pad = 8.0f;
    float avail_w = fmaxf(32.0f, b.w - 2.0f * pad);
    float avail_h = fmaxf(32.0f, b.h - 2.0f * pad);
    float aspect = 1.25f;
    float iw = avail_w;
    float ih = iw / aspect;
    if (ih > avail_h) { ih = avail_h; iw = ih * aspect; }
    struct nk_rect img_r = nk_rect(b.x + (b.w - iw) * 0.5f, b.y + (b.h - ih) * 0.5f, iw, ih);

    struct nk_image slice_img = nk_image_id((int)FD_TEX_SLICE);
    nk_draw_image(out, img_r, &slice_img, nk_rgba(255, 255, 255, 255));
    nk_stroke_rect(out, img_r, 3.0f, 1.5f, nk_rgba(88, 108, 142, 255));

    /* Draw Cold Spot (sky blue) and Peak Crust (amber) crosshairs when on/near active slice */
    if (sim && sim->initialized && sim->grid.count > 1) {
        const fd_grid_t *g = &sim->grid;
        const fd_metrics_t *m = &sim->metrics;
        int dim_u = (plane == FD_SLICE_YZ) ? g->ny : g->nx;
        int dim_v = (plane == FD_SLICE_XY) ? g->ny : g->nz;
        int dim_n = (plane == FD_SLICE_XY) ? g->nz : ((plane == FD_SLICE_XZ) ? g->ny : g->nx);
        int cur_n = (int)roundf(clampf(slice_norm, 0.0f, 1.0f) * (float)(dim_n > 1 ? dim_n - 1 : 0));

        int cu = (plane == FD_SLICE_YZ) ? m->cold_y : m->cold_x;
        int cv = (plane == FD_SLICE_XY) ? m->cold_y : m->cold_z;
        int cn = (plane == FD_SLICE_XY) ? m->cold_z : ((plane == FD_SLICE_XZ) ? m->cold_y : m->cold_x);
        if (abs(cn - cur_n) <= 1 && dim_u > 0 && dim_v > 0) {
            float cx = img_r.x + (((float)cu + 0.5f) / (float)dim_u) * img_r.w;
            float cy = img_r.y + (1.0f - (((float)cv + 0.5f) / (float)dim_v)) * img_r.h;
            nk_stroke_circle(out, nk_rect(cx - 7.0f, cy - 7.0f, 14.0f, 14.0f), 2.2f, nk_rgba(77, 163, 245, 255));
        }

        int hu = (plane == FD_SLICE_YZ) ? m->hot_y : m->hot_x;
        int hv = (plane == FD_SLICE_XY) ? m->hot_y : m->hot_z;
        int hn = (plane == FD_SLICE_XY) ? m->hot_z : ((plane == FD_SLICE_XZ) ? m->hot_y : m->hot_x);
        if (abs(hn - cur_n) <= 1 && dim_u > 0 && dim_v > 0) {
            float hx = img_r.x + (((float)hu + 0.5f) / (float)dim_u) * img_r.w;
            float hy = img_r.y + (1.0f - (((float)hv + 0.5f) / (float)dim_v)) * img_r.h;
            nk_stroke_circle(out, nk_rect(hx - 7.0f, hy - 7.0f, 14.0f, 14.0f), 2.2f, nk_rgba(245, 166, 52, 255));
        }
    }
}

void fd_ui_draw_thermal_chart(struct nk_context *ctx, const fd_sim_state_t *sim,
                              const char *lbl_surf, const char *lbl_cold, const char *lbl_mail,
                              float height) {
    nk_layout_row_dynamic(ctx, height, 1);
    struct nk_rect b;
    if (!nk_widget(&b, ctx)) return;
    struct nk_command_buffer *out = nk_window_get_canvas(ctx);

    nk_fill_rect(out, b, 8.0f, nk_rgba(16, 20, 30, 255));
    nk_stroke_rect(out, b, 8.0f, 1.0f, nk_rgba(48, 60, 82, 255));

    /* Color legend at top of chart */
    float ly = b.y + 5.0f;
    struct nk_color c_surf = nk_rgba(246, 130, 58, 255);
    struct nk_color c_cold = nk_rgba(77, 163, 245, 255);
    struct nk_color c_mail = nk_rgba(235, 196, 68, 255);
    float third = (b.w - 16.0f) / 3.0f;

    nk_fill_rect(out, nk_rect(b.x + 8.0f, ly + 4.0f, 10.0f, 4.0f), 2.0f, c_surf);
    nk_draw_text(out, nk_rect(b.x + 22.0f, ly, third - 16.0f, 16.0f),
                 lbl_surf, (int)strlen(lbl_surf), ctx->style.font, nk_rgba(0,0,0,0), c_surf);

    nk_fill_rect(out, nk_rect(b.x + 8.0f + third, ly + 4.0f, 10.0f, 4.0f), 2.0f, c_cold);
    nk_draw_text(out, nk_rect(b.x + 22.0f + third, ly, third - 16.0f, 16.0f),
                 lbl_cold, (int)strlen(lbl_cold), ctx->style.font, nk_rgba(0,0,0,0), c_cold);

    nk_fill_rect(out, nk_rect(b.x + 8.0f + 2.0f * third, ly + 4.0f, 10.0f, 4.0f), 2.0f, c_mail);
    nk_draw_text(out, nk_rect(b.x + 22.0f + 2.0f * third, ly, third - 16.0f, 16.0f),
                 lbl_mail, (int)strlen(lbl_mail), ctx->style.font, nk_rgba(0,0,0,0), c_mail);

    float px = b.x + 10.0f, py = b.y + 24.0f, pw = b.w - 20.0f, ph = fmaxf(20.0f, b.h - 32.0f);
    for (int g = 0; g <= 4; g++) {
        float gy = py + ph * ((float)g / 4.0f);
        nk_stroke_line(out, px, gy, px + pw, gy, 1.0f, nk_rgba(34, 43, 60, 255));
    }

    if (!sim || sim->hist_count < 2) return;
    int n = sim->hist_count;
    int start = (n == FD_HISTORY_LEN) ? sim->hist_head : 0;

    for (int i = 0; i < n - 1; i++) {
        int i0 = (start + i) % FD_HISTORY_LEN;
        int i1 = (start + i + 1) % FD_HISTORY_LEN;
        float x0 = px + ((float)i / (float)(n - 1)) * pw;
        float x1 = px + ((float)(i + 1) / (float)(n - 1)) * pw;

        float ys0 = py + ph * (1.0f - clampf(sim->hist_t_surf[i0] / 250.0f, 0.0f, 1.0f));
        float ys1 = py + ph * (1.0f - clampf(sim->hist_t_surf[i1] / 250.0f, 0.0f, 1.0f));
        nk_stroke_line(out, x0, ys0, x1, ys1, 2.2f, c_surf);

        float yc0 = py + ph * (1.0f - clampf(sim->hist_t_cold[i0] / 250.0f, 0.0f, 1.0f));
        float yc1 = py + ph * (1.0f - clampf(sim->hist_t_cold[i1] / 250.0f, 0.0f, 1.0f));
        nk_stroke_line(out, x0, yc0, x1, yc1, 2.2f, c_cold);

        float ym0 = py + ph * (1.0f - clampf(sim->hist_maillard[i0], 0.0f, 1.0f));
        float ym1 = py + ph * (1.0f - clampf(sim->hist_maillard[i1], 0.0f, 1.0f));
        nk_stroke_line(out, x0, ym0, x1, ym1, 2.0f, c_mail);
    }
}
