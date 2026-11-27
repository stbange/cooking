#ifndef FD_I18N_H
#define FD_I18N_H

#include "fd_lua.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FD_LANG_COUNT      20
#define FD_I18N_MAX_KEYS   256
#define FD_I18N_KEY_CAP    48
#define FD_I18N_VAL_CAP    512

typedef struct {
    const char *code;         /* ISO 639-1 code ("en", "zh", "ar", ...) */
    const char *english_name; /* ASCII English name */
    const char *native_name;  /* Native script display name */
    const char *font_path;    /* Optional secondary script font under ./assets/ */
    bool        is_rtl;       /* Arabic / Urdu visual RTL shaping */
    bool        is_indic;     /* Pre-base matra visual swapper */
} fd_lang_info_t;

typedef struct {
    char key[FD_I18N_KEY_CAP];
    char val[FD_I18N_VAL_CAP];
} fd_i18n_entry_t;

typedef struct {
    int              active_lang; /* 0..FD_LANG_COUNT-1 */
    int              en_count;
    fd_i18n_entry_t *en_entries;
    int              cur_count;
    fd_i18n_entry_t *cur_entries;
    bool             font_dirty;
} fd_i18n_t;

/* Catalog queries */
const fd_lang_info_t *fd_i18n_lang_info(int idx);
int  fd_i18n_find_lang(const char *code_or_bcp47);

/* Lifecycle & language switching */
bool fd_i18n_init(fd_i18n_t *i18n, fd_lua_t *lua, const char *initial_code);
void fd_i18n_free(fd_i18n_t *i18n);
bool fd_i18n_set_lang(fd_i18n_t *i18n, fd_lua_t *lua, int lang_idx);

/* Translate key (returns pre-shaped UTF-8 string; falls back to English, then key) */
const char *fd_tr(const fd_i18n_t *i18n, const char *key);

/* Label for language picker safe against tofu boxes for currently loaded script font */
void fd_i18n_lang_menu_label(const fd_i18n_t *i18n, int target_idx, char *out, size_t out_cap);

/* Shape UTF-8 text on demand (Arabic/Urdu contextual forms + RTL BiDi, Indic pre-base matras) */
void fd_i18n_shape_text(const fd_i18n_t *i18n, const char *utf8_in, char *out, size_t out_cap);

/* UTF-8 decode/encode helpers */
int  fd_utf8_decode(const char *s, uint32_t *out_cp);
int  fd_utf8_encode(uint32_t cp, char out[4]);

/* Build zero-terminated Nuklear rune range array [start, end, ..., 0] for active locale */
int  fd_i18n_build_rune_ranges(const fd_i18n_t *i18n, uint32_t *out_ranges, int max_u32);

#endif /* FD_I18N_H */
