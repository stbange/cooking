#include "fd_i18n.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const fd_lang_info_t g_langs[FD_LANG_COUNT] = {
    { "en", "English",          "English",          NULL,                           false, false },
    { "zh", "Mandarin Chinese", "\xe4\xb8\xad\xe6\x96\x87 (\xe7\xae\x80\xe4\xbd\x93)", "fonts/NotoSans-CJK-SC.otf", false, false },
    { "hi", "Hindi",            "\xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\xa8\xe0\xa5\x8d\xe0\xa4\xa6\xe0\xa5\x80", "fonts/NotoSans-Devanagari.ttf", false, true },
    { "es", "Spanish",          "Espa\xc3\xb1" "ol", NULL,                           false, false },
    { "fr", "French",           "Fran\xc3\xa7" "ais", NULL,                          false, false },
    { "ar", "Arabic",           "\xd8\xa7\xd9\x84\xd8\xb9\xd8\xb1\xd8\xa8\xd9\x8a\xd8\xa9", "fonts/NotoSans-Arabic.ttf", true, false },
    { "bn", "Bengali",          "\xe0\xa6\xac\xe0\xa6\xbe\xe0\xa6\x82\xe0\xa6\xb2\xe0\xa6\xbe", "fonts/NotoSans-Bengali.ttf", false, true },
    { "pt", "Portuguese",       "Portugu\xc3\xaa" "s", NULL,                         false, false },
    { "ru", "Russian",          "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9", NULL, false, false },
    { "ur", "Urdu",             "\xd8\xa7\xd8\xb1\xd8\xaf\xd9\x88", "fonts/NotoSans-Arabic.ttf", true, false },
    { "id", "Indonesian",       "Bahasa Indonesia", NULL,                           false, false },
    { "de", "German",           "Deutsch",          NULL,                           false, false },
    { "ja", "Japanese",         "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", "fonts/NotoSans-CJK-JP.otf", false, false },
    { "sw", "Swahili",          "Kiswahili",        NULL,                           false, false },
    { "mr", "Marathi",          "\xe0\xa4\xae\xe0\xa4\xb0\xe0\xa4\xbe\xe0\xa4\xa0\xe0\xa5\x80", "fonts/NotoSans-Devanagari.ttf", false, true },
    { "te", "Telugu",           "\xe0\xb0\xa4\xe0\xb1\x86\xe0\xb0\xb2\xe0\xb1\x81\xe0\xb0\x97\xe0\xb1\x81", "fonts/NotoSans-Telugu.ttf", false, true },
    { "tr", "Turkish",          "T\xc3\xbcrk\xc3\xa7" "e", NULL,                    false, false },
    { "ta", "Tamil",            "\xe0\xae\xa4\xe0\xae\xae\xe0\xae\xbf\xe0\xae\xb4\xe0\xaf\x8d", "fonts/NotoSans-Tamil.ttf", false, true },
    { "vi", "Vietnamese",       "Ti\xe1\xba\xbfng Vi\xe1\xbb\x87t", NULL,           false, false },
    { "ko", "Korean",           "\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4", "fonts/NotoSans-CJK-KR.otf", false, false }
};

const fd_lang_info_t *fd_i18n_lang_info(int idx) {
    if (idx < 0 || idx >= FD_LANG_COUNT) return &g_langs[0];
    return &g_langs[idx];
}

int fd_i18n_find_lang(const char *code_or_bcp47) {
    if (!code_or_bcp47 || code_or_bcp47[0] == '\0') return 0;
    char lc[4] = { 0, 0, 0, 0 };
    lc[0] = (char)tolower((unsigned char)code_or_bcp47[0]);
    if (code_or_bcp47[1]) lc[1] = (char)tolower((unsigned char)code_or_bcp47[1]);
    for (int i = 0; i < FD_LANG_COUNT; i++) {
        if (strcmp(g_langs[i].code, lc) == 0) return i;
    }
    return 0;
}

int fd_utf8_decode(const char *s, uint32_t *out_cp) {
    const unsigned char *p = (const unsigned char *)s;
    if (!p || !p[0]) {
        if (out_cp) *out_cp = 0;
        return 0;
    }
    if (p[0] < 0x80u) {
        if (out_cp) *out_cp = p[0];
        return 1;
    }
    if ((p[0] & 0xE0u) == 0xC0u && (p[1] & 0xC0u) == 0x80u) {
        if (out_cp) *out_cp = ((uint32_t)(p[0] & 0x1Fu) << 6) | (uint32_t)(p[1] & 0x3Fu);
        return 2;
    }
    if ((p[0] & 0xF0u) == 0xE0u && (p[1] & 0xC0u) == 0x80u && (p[2] & 0xC0u) == 0x80u) {
        if (out_cp) {
            *out_cp = ((uint32_t)(p[0] & 0x0Fu) << 12)
                    | ((uint32_t)(p[1] & 0x3Fu) << 6)
                    |  (uint32_t)(p[2] & 0x3Fu);
        }
        return 3;
    }
    if ((p[0] & 0xF8u) == 0xF0u && (p[1] & 0xC0u) == 0x80u &&
        (p[2] & 0xC0u) == 0x80u && (p[3] & 0xC0u) == 0x80u) {
        if (out_cp) {
            *out_cp = ((uint32_t)(p[0] & 0x07u) << 18)
                    | ((uint32_t)(p[1] & 0x3Fu) << 12)
                    | ((uint32_t)(p[2] & 0x3Fu) << 6)
                    |  (uint32_t)(p[3] & 0x3Fu);
        }
        return 4;
    }
    if (out_cp) *out_cp = 0xFFFDu;
    return 1;
}

int fd_utf8_encode(uint32_t cp, char out[4]) {
    if (cp < 0x80u) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800u) {
        out[0] = (char)(0xC0u | (cp >> 6));
        out[1] = (char)(0x80u | (cp & 0x3Fu));
        return 2;
    }
    if (cp < 0x10000u) {
        out[0] = (char)(0xE0u | (cp >> 12));
        out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (char)(0x80u | (cp & 0x3Fu));
        return 3;
    }
    out[0] = (char)(0xF0u | (cp >> 18));
    out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
    out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
    out[3] = (char)(0x80u | (cp & 0x3Fu));
    return 4;
}

/* Arabic & Urdu contextual presentation form table (Unicode Presentation Forms-A/B) */
typedef struct {
    uint16_t cp, iso, fin, ini, med;
} ar_form_t;

static const ar_form_t g_ar_forms[] = {
    { 0x0621, 0xFE80, 0x0000, 0x0000, 0x0000 }, /* HAMZA */
    { 0x0622, 0xFE81, 0xFE82, 0x0000, 0x0000 }, /* ALEF MADDA */
    { 0x0623, 0xFE83, 0xFE84, 0x0000, 0x0000 }, /* ALEF HAMZA ABOVE */
    { 0x0624, 0xFE85, 0xFE86, 0x0000, 0x0000 }, /* WAW HAMZA */
    { 0x0625, 0xFE87, 0xFE88, 0x0000, 0x0000 }, /* ALEF HAMZA BELOW */
    { 0x0626, 0xFE89, 0xFE8A, 0xFE8B, 0xFE8C }, /* YEH HAMZA */
    { 0x0627, 0xFE8D, 0xFE8E, 0x0000, 0x0000 }, /* ALEF */
    { 0x0628, 0xFE8F, 0xFE90, 0xFE91, 0xFE92 }, /* BEH */
    { 0x0629, 0xFE93, 0xFE94, 0x0000, 0x0000 }, /* TEH MARBUTA */
    { 0x062A, 0xFE95, 0xFE96, 0xFE97, 0xFE98 }, /* TEH */
    { 0x062B, 0xFE99, 0xFE9A, 0xFE9B, 0xFE9C }, /* THEH */
    { 0x062C, 0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0 }, /* JEEM */
    { 0x062D, 0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4 }, /* HAH */
    { 0x062E, 0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8 }, /* KHAH */
    { 0x062F, 0xFEA9, 0xFEAA, 0x0000, 0x0000 }, /* DAL */
    { 0x0630, 0xFEAB, 0xFEAC, 0x0000, 0x0000 }, /* THAL */
    { 0x0631, 0xFEAD, 0xFEAE, 0x0000, 0x0000 }, /* REH */
    { 0x0632, 0xFEAF, 0xFEB0, 0x0000, 0x0000 }, /* ZAIN */
    { 0x0633, 0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4 }, /* SEEN */
    { 0x0634, 0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8 }, /* SHEEN */
    { 0x0635, 0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC }, /* SAD */
    { 0x0636, 0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0 }, /* DAD */
    { 0x0637, 0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4 }, /* TAH */
    { 0x0638, 0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8 }, /* ZAH */
    { 0x0639, 0xFEC9, 0xFECA, 0xFECB, 0xFECC }, /* AIN */
    { 0x063A, 0xFECD, 0xFECE, 0xFECF, 0xFED0 }, /* GHAIN */
    { 0x0641, 0xFED1, 0xFED2, 0xFED3, 0xFED4 }, /* FEH */
    { 0x0642, 0xFED5, 0xFED6, 0xFED7, 0xFED8 }, /* QAF */
    { 0x0643, 0xFED9, 0xFEDA, 0xFEDB, 0xFEDC }, /* KAF */
    { 0x0644, 0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0 }, /* LAM */
    { 0x0645, 0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4 }, /* MEEM */
    { 0x0646, 0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8 }, /* NOON */
    { 0x0647, 0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC }, /* HEH */
    { 0x0648, 0xFEED, 0xFEEE, 0x0000, 0x0000 }, /* WAW */
    { 0x0649, 0xFEEF, 0xFEF0, 0x0000, 0x0000 }, /* ALEF MAKSURA */
    { 0x064A, 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4 }, /* YEH */
    /* Urdu / Perso-Arabic letters */
    { 0x0679, 0xFB66, 0xFB67, 0xFB68, 0xFB69 }, /* TTEH */
    { 0x067E, 0xFB56, 0xFB57, 0xFB58, 0xFB59 }, /* PEH */
    { 0x0686, 0xFB7A, 0xFB7B, 0xFB7C, 0xFB7D }, /* TCHEH */
    { 0x0688, 0xFB88, 0xFB89, 0x0000, 0x0000 }, /* DDAL */
    { 0x0691, 0xFB8C, 0xFB8D, 0x0000, 0x0000 }, /* RREH */
    { 0x0698, 0xFB8A, 0xFB8B, 0x0000, 0x0000 }, /* JEH */
    { 0x06A9, 0xFB8E, 0xFB8F, 0xFB90, 0xFB91 }, /* KEHEH */
    { 0x06AF, 0xFB92, 0xFB93, 0xFB94, 0xFB95 }, /* GAF */
    { 0x06BA, 0xFB9E, 0xFB9F, 0x0000, 0x0000 }, /* NOON GHUNNA */
    { 0x06BE, 0xFBAA, 0xFBAB, 0xFBAC, 0xFBAD }, /* HEH DOACHASHMEE */
    { 0x06C1, 0xFBA6, 0xFBA7, 0xFBA8, 0xFBA9 }, /* HEH GOAL */
    { 0x06CC, 0xFBFC, 0xFBFD, 0xFBFE, 0xFBFF }, /* FARSI YEH */
    { 0x06D2, 0xFBAE, 0xFBAF, 0x0000, 0x0000 }  /* YEH BARREE */
};

static const ar_form_t *find_ar_form(uint32_t cp) {
    for (size_t i = 0; i < sizeof(g_ar_forms) / sizeof(g_ar_forms[0]); i++) {
        if (g_ar_forms[i].cp == cp) return &g_ar_forms[i];
    }
    return NULL;
}

static bool ar_joins_left(uint32_t cp) {
    const ar_form_t *f = find_ar_form(cp);
    return f != NULL && f->ini != 0;
}

static bool ar_joins_right(uint32_t cp) {
    const ar_form_t *f = find_ar_form(cp);
    return f != NULL && f->fin != 0;
}

static bool is_ltr_token_char(uint32_t cp) {
    return (cp >= '0' && cp <= '9') || (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') ||
           cp == '.' || cp == '%' || cp == 0x00B0u || cp == '+' || cp == '-' ||
           cp == '/' || cp == '=' || cp == '_' || cp == '>';
}

static uint32_t mirror_bracket(uint32_t cp) {
    if (cp == '(') return ')';
    if (cp == ')') return '(';
    if (cp == '[') return ']';
    if (cp == ']') return '[';
    return cp;
}

static void reverse_u32(uint32_t *arr, int a, int b) {
    while (a < b) {
        uint32_t t = arr[a];
        arr[a] = arr[b];
        arr[b] = t;
        a++; b--;
    }
}

static int shape_arabic_line(const uint32_t *in, int n, uint32_t *out, int cap) {
    bool has_ar = false;
    for (int i = 0; i < n; i++) {
        if (in[i] >= 0x0600u && in[i] <= 0x06FFu) { has_ar = true; break; }
    }
    if (!has_ar) {
        int m = n < cap ? n : cap;
        memcpy(out, in, (size_t)m * sizeof(uint32_t));
        return m;
    }

    /* 1. Contextual glyph substitution + Lam-Alef ligatures */
    int m = 0;
    for (int i = 0; i < n && m < cap; i++) {
        uint32_t c = in[i];
        bool prev_l = (i > 0) && ar_joins_left(in[i - 1]);
        if (c == 0x0644u && i + 1 < n) {
            uint32_t nx = in[i + 1];
            uint32_t lig = 0;
            if (nx == 0x0622u) lig = prev_l ? 0xFEF6u : 0xFEF5u;
            else if (nx == 0x0623u) lig = prev_l ? 0xFEF8u : 0xFEF7u;
            else if (nx == 0x0625u) lig = prev_l ? 0xFEFAu : 0xFEF9u;
            else if (nx == 0x0627u) lig = prev_l ? 0xFEFCu : 0xFEFBu;
            if (lig) {
                out[m++] = lig;
                i++;
                continue;
            }
        }
        const ar_form_t *f = find_ar_form(c);
        if (!f) {
            out[m++] = mirror_bracket(c);
            continue;
        }
        bool next_r = (i + 1 < n) && ar_joins_right(in[i + 1]);
        if (prev_l && next_r && f->med) out[m++] = f->med;
        else if (prev_l && f->fin)      out[m++] = f->fin;
        else if (next_r && f->ini)      out[m++] = f->ini;
        else                            out[m++] = f->iso;
    }

    /* 2. Visual RTL reversal while keeping embedded LTR numbers/units readable */
    reverse_u32(out, 0, m - 1);
    int i = 0;
    while (i < m) {
        if (is_ltr_token_char(out[i])) {
            int start = i;
            bool has_alnum = false;
            while (i < m && (is_ltr_token_char(out[i]) || (out[i] == ' ' && i + 1 < m && is_ltr_token_char(out[i + 1])))) {
                if ((out[i] >= '0' && out[i] <= '9') || (out[i] >= 'A' && out[i] <= 'Z') ||
                    (out[i] >= 'a' && out[i] <= 'z')) {
                    has_alnum = true;
                }
                i++;
            }
            int end = i - 1;
            while (end > start && out[end] == ' ') end--;
            if (has_alnum && end > start) reverse_u32(out, start, end);
        } else {
            i++;
        }
    }
    return m;
}

/* shortcut: reorders pre-base Indic vowel signs visually without HarfBuzz; upgrade to HarfBuzz for full GSUB conjunct ligatures */
static bool is_prebase_indic_matra(uint32_t cp) {
    return cp == 0x093Fu || cp == 0x09BFu || cp == 0x09C7u || cp == 0x09C8u ||
           cp == 0x0BC6u || cp == 0x0BC7u || cp == 0x0BC8u;
}

static void shape_indic_line(uint32_t *cps, int n) {
    for (int i = 1; i < n; i++) {
        if (is_prebase_indic_matra(cps[i])) {
            uint32_t matra = cps[i];
            cps[i] = cps[i - 1];
            cps[i - 1] = matra;
        }
    }
}

static void shape_utf8_for_lang(const fd_lang_info_t *lang, const char *utf8_in, char *out, size_t out_cap) {
    if (!out || out_cap == 0) return;
    if (!utf8_in) { out[0] = '\0'; return; }
    if (!lang || (!lang->is_rtl && !lang->is_indic)) {
        snprintf(out, out_cap, "%s", utf8_in);
        return;
    }

    uint32_t line_in[512];
    uint32_t line_out[512];
    size_t pos = 0;
    const char *p = utf8_in;

    while (*p && pos + 1 < out_cap) {
        int n = 0;
        while (*p && *p != '\n' && n < 512) {
            uint32_t cp = 0;
            int step = fd_utf8_decode(p, &cp);
            if (step <= 0) break;
            line_in[n++] = cp;
            p += step;
        }
        int out_n = n;
        if (lang->is_rtl) {
            out_n = shape_arabic_line(line_in, n, line_out, 512);
        } else {
            memcpy(line_out, line_in, (size_t)n * sizeof(uint32_t));
            shape_indic_line(line_out, out_n);
        }
        for (int i = 0; i < out_n; i++) {
            char enc[4];
            int eb = fd_utf8_encode(line_out[i], enc);
            if (pos + (size_t)eb >= out_cap) break;
            memcpy(out + pos, enc, (size_t)eb);
            pos += (size_t)eb;
        }
        if (*p == '\n' && pos + 1 < out_cap) {
            out[pos++] = '\n';
            p++;
        }
    }
    out[pos] = '\0';
}

void fd_i18n_shape_text(const fd_i18n_t *i18n, const char *utf8_in, char *out, size_t out_cap) {
    const fd_lang_info_t *lang = i18n ? fd_i18n_lang_info(i18n->active_lang) : &g_langs[0];
    shape_utf8_for_lang(lang, utf8_in, out, out_cap);
}

typedef struct {
    fd_i18n_entry_t      *entries;
    int                  *count;
    const fd_lang_info_t *lang;
} load_ctx_t;

static void on_locale_kv(const char *key, const char *val, void *ud) {
    load_ctx_t *ctx = (load_ctx_t *)ud;
    if (!key || !val || key[0] == '\0') return;
    for (int i = 0; i < *ctx->count; i++) {
        if (strcmp(ctx->entries[i].key, key) == 0) {
            shape_utf8_for_lang(ctx->lang, val, ctx->entries[i].val, FD_I18N_VAL_CAP);
            return;
        }
    }
    if (*ctx->count >= FD_I18N_MAX_KEYS) return;
    fd_i18n_entry_t *e = &ctx->entries[(*ctx->count)++];
    snprintf(e->key, FD_I18N_KEY_CAP, "%s", key);
    shape_utf8_for_lang(ctx->lang, val, e->val, FD_I18N_VAL_CAP);
}

bool fd_i18n_init(fd_i18n_t *i18n, fd_lua_t *lua, const char *initial_code) {
    if (!i18n || !lua) return false;
    memset(i18n, 0, sizeof(*i18n));
    i18n->en_entries  = (fd_i18n_entry_t *)calloc(FD_I18N_MAX_KEYS, sizeof(fd_i18n_entry_t));
    i18n->cur_entries = (fd_i18n_entry_t *)calloc(FD_I18N_MAX_KEYS, sizeof(fd_i18n_entry_t));
    if (!i18n->en_entries || !i18n->cur_entries) {
        fd_i18n_free(i18n);
        return false;
    }

    load_ctx_t en_ctx = { i18n->en_entries, &i18n->en_count, &g_langs[0] };
    fd_lua_load_string_table(lua, "locales/en.lua", on_locale_kv, &en_ctx);

    int lang_idx = fd_i18n_find_lang(initial_code);
    return fd_i18n_set_lang(i18n, lua, lang_idx);
}

void fd_i18n_free(fd_i18n_t *i18n) {
    if (!i18n) return;
    free(i18n->en_entries);
    free(i18n->cur_entries);
    memset(i18n, 0, sizeof(*i18n));
}

bool fd_i18n_set_lang(fd_i18n_t *i18n, fd_lua_t *lua, int lang_idx) {
    if (!i18n || !lua || !i18n->cur_entries) return false;
    if (lang_idx < 0 || lang_idx >= FD_LANG_COUNT) lang_idx = 0;

    i18n->active_lang = lang_idx;
    i18n->cur_count   = 0;
    i18n->font_dirty  = true;

    if (lang_idx == 0) {
        memcpy(i18n->cur_entries, i18n->en_entries, (size_t)i18n->en_count * sizeof(fd_i18n_entry_t));
        i18n->cur_count = i18n->en_count;
        return i18n->en_count > 0;
    }

    char path[64];
    snprintf(path, sizeof(path), "locales/%s.lua", g_langs[lang_idx].code);
    load_ctx_t ctx = { i18n->cur_entries, &i18n->cur_count, &g_langs[lang_idx] };
    return fd_lua_load_string_table(lua, path, on_locale_kv, &ctx);
}

const char *fd_tr(const fd_i18n_t *i18n, const char *key) {
    if (!key) return "";
    if (!i18n) return key;
    if (i18n->cur_entries) {
        for (int i = 0; i < i18n->cur_count; i++) {
            if (strcmp(i18n->cur_entries[i].key, key) == 0) return i18n->cur_entries[i].val;
        }
    }
    if (i18n->en_entries) {
        for (int i = 0; i < i18n->en_count; i++) {
            if (strcmp(i18n->en_entries[i].key, key) == 0) return i18n->en_entries[i].val;
        }
    }
    return key;
}

void fd_i18n_lang_menu_label(const fd_i18n_t *i18n, int target_idx, char *out, size_t out_cap) {
    if (!out || out_cap == 0) return;
    const fd_lang_info_t *act = fd_i18n_lang_info(i18n ? i18n->active_lang : 0);
    const fd_lang_info_t *tgt = fd_i18n_lang_info(target_idx);
    char code_up[4] = {
        (char)toupper((unsigned char)tgt->code[0]),
        (char)toupper((unsigned char)tgt->code[1]),
        '\0', '\0'
    };
    bool same_font = (tgt->font_path == NULL) ||
                     (act->font_path != NULL && strcmp(act->font_path, tgt->font_path) == 0);
    if (same_font) {
        char shaped[96];
        shape_utf8_for_lang(tgt, tgt->native_name, shaped, sizeof(shaped));
        snprintf(out, out_cap, "[%s] %s", code_up, shaped);
    } else {
        snprintf(out, out_cap, "[%s] %s", code_up, tgt->english_name);
    }
}

static int cmp_u32(const void *a, const void *b) {
    uint32_t va = *(const uint32_t *)a;
    uint32_t vb = *(const uint32_t *)b;
    return (va < vb) ? -1 : ((va > vb) ? 1 : 0);
}

static void collect_str_cps(const char *s, uint32_t *buf, int *n, int cap, uint32_t min_cp) {
    if (!s) return;
    const char *p = s;
    while (*p && *n < cap) {
        uint32_t cp = 0;
        int step = fd_utf8_decode(p, &cp);
        if (step <= 0) break;
        if (cp >= min_cp && cp != 0xFFFDu) buf[(*n)++] = cp;
        p += step;
    }
}

int fd_i18n_build_rune_ranges(const fd_i18n_t *i18n, uint32_t *out_ranges, int max_u32) {
    if (!i18n || !out_ranges || max_u32 < 3) return 0;
    uint32_t cps[4096];
    int n = 0;
    const fd_lang_info_t *act = fd_i18n_lang_info(i18n->active_lang);

    /* Collect secondary-script codepoints (>= 0x0500) used by active locale + matching menu labels */
    for (int i = 0; i < i18n->cur_count; i++) {
        collect_str_cps(i18n->cur_entries[i].val, cps, &n, 4096, 0x0500u);
    }
    if (act->font_path != NULL) {
        for (int i = 0; i < FD_LANG_COUNT; i++) {
            if (g_langs[i].font_path && strcmp(g_langs[i].font_path, act->font_path) == 0) {
                char shaped[96];
                shape_utf8_for_lang(&g_langs[i], g_langs[i].native_name, shaped, sizeof(shaped));
                collect_str_cps(shaped, cps, &n, 4096, 0x0500u);
            }
        }
    }

    if (n == 0) {
        out_ranges[0] = 0;
        return 0;
    }

    qsort(cps, (size_t)n, sizeof(uint32_t), cmp_u32);
    int out_idx = 0;
    uint32_t lo = cps[0], hi = cps[0];
    for (int i = 1; i < n; i++) {
        if (cps[i] == hi) continue;
        if (cps[i] == hi + 1u) {
            hi = cps[i];
        } else {
            if (out_idx + 3 <= max_u32) {
                out_ranges[out_idx++] = lo;
                out_ranges[out_idx++] = hi;
            }
            lo = hi = cps[i];
        }
    }
    if (out_idx + 3 <= max_u32) {
        out_ranges[out_idx++] = lo;
        out_ranges[out_idx++] = hi;
    }
    out_ranges[out_idx] = 0;
    return out_idx / 2;
}
