#include "fd_lua.h"

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FD_LUA_REG_KEY "fd_lua_host"
#define FD_MOD_LIST_KEY "fd_mod_list"

static void *sandbox_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    fd_lua_t *host = (fd_lua_t *)ud;
    size_t prev = ptr ? osize : 0;
    if (nsize == 0) {
        if (host->mem_used >= prev) host->mem_used -= prev;
        free(ptr);
        return NULL;
    }
    if (nsize > prev && (host->mem_used + (nsize - prev)) > host->mem_limit) {
        return NULL;
    }
    void *nptr = realloc(ptr, nsize);
    if (nptr) {
        host->mem_used = host->mem_used - prev + nsize;
    }
    return nptr;
}

static void instr_limit_hook(lua_State *L, lua_Debug *ar) {
    (void)ar;
    lua_getfield(L, LUA_REGISTRYINDEX, FD_LUA_REG_KEY);
    fd_lua_t *host = (fd_lua_t *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (host) {
        host->instr_budget -= 1000;
        if (host->instr_budget <= 0) {
            luaL_error(L, "Lua sandbox instruction quota exceeded");
        }
    }
}

static fd_lua_t *get_host(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, FD_LUA_REG_KEY);
    fd_lua_t *h = (fd_lua_t *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return h;
}

static float tbl_float(lua_State *L, int idx, const char *key, float def) {
    lua_getfield(L, idx, key);
    float v = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : def;
    lua_pop(L, 1);
    return v;
}

static double tbl_double(lua_State *L, int idx, const char *key, double def) {
    lua_getfield(L, idx, key);
    double v = lua_isnumber(L, -1) ? (double)lua_tonumber(L, -1) : def;
    lua_pop(L, 1);
    return v;
}

static bool tbl_bool(lua_State *L, int idx, const char *key, bool def) {
    lua_getfield(L, idx, key);
    bool v = lua_isboolean(L, -1) ? (lua_toboolean(L, -1) != 0) : def;
    lua_pop(L, 1);
    return v;
}

static void tbl_str(lua_State *L, int idx, const char *key, char *dst, size_t cap, const char *def) {
    lua_getfield(L, idx, key);
    const char *s = lua_isstring(L, -1) ? lua_tostring(L, -1) : def;
    snprintf(dst, cap, "%s", s ? s : "");
    lua_pop(L, 1);
}

static fd_shape_type_t parse_shape(const char *s) {
    if (!s) return FD_SHAPE_SLAB;
    if (strcmp(s, "liquid") == 0)   return FD_SHAPE_LIQUID;
    if (strcmp(s, "cube") == 0)     return FD_SHAPE_CUBE;
    if (strcmp(s, "cylinder") == 0) return FD_SHAPE_CYLINDER;
    if (strcmp(s, "sphere") == 0)   return FD_SHAPE_SPHERE;
    if (strcmp(s, "tapered") == 0)  return FD_SHAPE_TAPERED_FILLET;
    if (strcmp(s, "fat_cap") == 0)  return FD_SHAPE_FAT_CAP_SLAB;
    if (strcmp(s, "bone_in") == 0)  return FD_SHAPE_BONE_IN_SLAB;
    if (strcmp(s, "egg") == 0)      return FD_SHAPE_CONCENTRIC_EGG;
    return FD_SHAPE_SLAB;
}

static uint32_t parse_diet_flags(lua_State *L, int idx) {
    uint32_t flags = 0;
    lua_getfield(L, idx, "diet");
    if (lua_istable(L, -1)) {
        int n = (int)luaL_len(L, -1);
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, -1, i);
            const char *t = lua_tostring(L, -1);
            if (t) {
                if (strcmp(t, "vegan") == 0)       flags |= FD_DIET_VEGAN | FD_DIET_VEGETARIAN | FD_DIET_DAIRY_FREE;
                if (strcmp(t, "vegetarian") == 0)  flags |= FD_DIET_VEGETARIAN;
                if (strcmp(t, "gluten_free") == 0) flags |= FD_DIET_GLUTEN_FREE;
                if (strcmp(t, "dairy_free") == 0)  flags |= FD_DIET_DAIRY_FREE;
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return flags;
}

static uint32_t parse_role_flags(lua_State *L, int idx) {
    uint32_t flags = 0;
    lua_getfield(L, idx, "roles");
    if (lua_istable(L, -1)) {
        int n = (int)luaL_len(L, -1);
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, -1, i);
            const char *t = lua_tostring(L, -1);
            if (t) {
                if (strcmp(t, "protein_main") == 0) flags |= FD_ROLE_PROTEIN_MAIN;
                if (strcmp(t, "veg_main") == 0)     flags |= FD_ROLE_VEG_MAIN;
                if (strcmp(t, "starch_base") == 0)  flags |= FD_ROLE_STARCH_BASE;
                if (strcmp(t, "fat_medium") == 0)   flags |= FD_ROLE_FAT_MEDIUM;
                if (strcmp(t, "liquid_base") == 0)  flags |= FD_ROLE_LIQUID_BASE;
                if (strcmp(t, "acid") == 0)         flags |= FD_ROLE_ACID;
                if (strcmp(t, "umami") == 0)        flags |= FD_ROLE_UMAMI;
                if (strcmp(t, "sweetener") == 0)    flags |= FD_ROLE_SWEETENER;
                if (strcmp(t, "emulsifier") == 0)   flags |= FD_ROLE_EMULSIFIER;
                if (strcmp(t, "aromatic") == 0)     flags |= FD_ROLE_AROMATIC;
                if (strcmp(t, "salt") == 0)         flags |= FD_ROLE_SALT;
                if (strcmp(t, "leavener") == 0)     flags |= FD_ROLE_LEAVENER;
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return flags;
}

int fd_db_find_ingredient(const fd_database_t *db, const char *id) {
    if (!db || !id) return -1;
    for (int i = 0; i < db->ingredient_count; i++) {
        if (strcmp(db->ingredients[i].id, id) == 0) return i;
    }
    return -1;
}

int fd_db_find_technique(const fd_database_t *db, const char *id) {
    if (!db || !id) return -1;
    for (int i = 0; i < db->technique_count; i++) {
        if (strcmp(db->techniques[i].id, id) == 0) return i;
    }
    return -1;
}

int fd_db_find_recipe(const fd_database_t *db, const char *id) {
    if (!db || !id) return -1;
    for (int i = 0; i < db->recipe_count; i++) {
        if (strcmp(db->recipes[i].id, id) == 0) return i;
    }
    return -1;
}

static int l_include(lua_State *L) {
    const char *rel = luaL_checkstring(L, 1);
    fd_lua_t *h = get_host(L);
    if (!h || !h->vfs || !fd_vfs_valid_relpath(rel)) {
        return luaL_error(L, "invalid script path: %s", rel ? rel : "(null)");
    }
    uint8_t *buf = NULL;
    size_t sz = 0;
    if (!fd_vfs_read(h->vfs, rel, &buf, &sz)) {
        return luaL_error(L, "cannot read script: %s", rel);
    }
    char chunk[160];
    snprintf(chunk, sizeof(chunk), "@%s", rel);
    int st = luaL_loadbuffer(L, (const char *)buf, sz, chunk);
    free(buf);
    if (st != LUA_OK) return lua_error(L);
    lua_call(L, 0, LUA_MULTRET);
    return 0;
}

static int l_set_reactions(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    fd_lua_t *h = get_host(L);
    fd_reactions_t *r = &h->db->reactions;
    r->maillard.a     = tbl_double(L, 1, "maillard_a", r->maillard.a);
    r->maillard.ea    = tbl_double(L, 1, "maillard_ea", r->maillard.ea);
    r->caramel.a      = tbl_double(L, 1, "caramel_a", r->caramel.a);
    r->caramel.ea     = tbl_double(L, 1, "caramel_ea", r->caramel.ea);
    r->pyrolysis.a    = tbl_double(L, 1, "pyrolysis_a", r->pyrolysis.a);
    r->pyrolysis.ea   = tbl_double(L, 1, "pyrolysis_ea", r->pyrolysis.ea);
    r->myosin.a       = tbl_double(L, 1, "myosin_a", r->myosin.a);
    r->myosin.ea      = tbl_double(L, 1, "myosin_ea", r->myosin.ea);
    r->actin.a        = tbl_double(L, 1, "actin_a", r->actin.a);
    r->actin.ea       = tbl_double(L, 1, "actin_ea", r->actin.ea);
    r->albumen.a      = tbl_double(L, 1, "albumen_a", r->albumen.a);
    r->albumen.ea     = tbl_double(L, 1, "albumen_ea", r->albumen.ea);
    r->collagen.a     = tbl_double(L, 1, "collagen_a", r->collagen.a);
    r->collagen.ea    = tbl_double(L, 1, "collagen_ea", r->collagen.ea);
    r->starch_gel.a   = tbl_double(L, 1, "starch_gel_a", r->starch_gel.a);
    r->starch_gel.ea  = tbl_double(L, 1, "starch_gel_ea", r->starch_gel.ea);
    r->retrogradation_rate  = tbl_float(L, 1, "retrogradation_rate", r->retrogradation_rate);
    r->target_pasteur_log10 = tbl_float(L, 1, "target_pasteur_log10", r->target_pasteur_log10);
    r->umami_synergy_gamma  = tbl_float(L, 1, "umami_synergy_gamma", r->umami_synergy_gamma);
    return 0;
}

static int l_add_ingredient(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    fd_lua_t *h = get_host(L);
    fd_ingredient_t ing;
    memset(&ing, 0, sizeof(ing));
    tbl_str(L, 1, "id", ing.id, sizeof(ing.id), "");
    if (ing.id[0] == '\0') return luaL_error(L, "ingredient missing id");
    tbl_str(L, 1, "name", ing.name, sizeof(ing.name), ing.id);
    tbl_str(L, 1, "category", ing.category, sizeof(ing.category), "misc");

    ing.water   = tbl_float(L, 1, "water", 0.70f);
    ing.protein = tbl_float(L, 1, "protein", 0.05f);
    ing.fat     = tbl_float(L, 1, "fat", 0.02f);
    ing.starch  = tbl_float(L, 1, "starch", 0.0f);
    ing.sugar   = tbl_float(L, 1, "sugar", 0.01f);
    ing.fiber   = tbl_float(L, 1, "fiber", 0.0f);
    ing.ash     = tbl_float(L, 1, "ash", 0.01f);

    ing.collagen_frac       = tbl_float(L, 1, "collagen_frac", 0.02f);
    ing.reducing_sugar_frac = tbl_float(L, 1, "reducing_sugar_frac", 0.5f);
    ing.free_amino          = tbl_float(L, 1, "free_amino", ing.protein > 0.1f ? 0.6f : 0.15f);
    ing.ph                  = tbl_float(L, 1, "ph", 6.2f);
    ing.acidity_molar       = tbl_float(L, 1, "acidity_molar", 0.01f);
    ing.sodium_mg_100g      = tbl_float(L, 1, "sodium_mg_100g", 40.0f);
    ing.glutamate_mg_100g   = tbl_float(L, 1, "glutamate_mg_100g", 20.0f);
    ing.nucleotides_mg_100g = tbl_float(L, 1, "nucleotides_mg_100g", 0.0f);
    ing.emulsifier_power    = tbl_float(L, 1, "emulsifier_power", 0.0f);
    ing.smoke_point_c       = tbl_float(L, 1, "smoke_point_c", 220.0f);
    ing.pungency            = tbl_float(L, 1, "pungency", 0.0f);
    ing.aroma_potency       = tbl_float(L, 1, "aroma_potency", 0.25f);
    ing.bitter_base         = tbl_float(L, 1, "bitter_base", 0.05f);

    ing.amylase_activity        = tbl_float(L, 1, "amylase_activity", 0.0f);
    ing.yeast_activity          = tbl_float(L, 1, "yeast_activity", 0.0f);
    ing.lacto_activity          = tbl_float(L, 1, "lacto_activity", 0.0f);
    ing.antinutrient_frac       = tbl_float(L, 1, "antinutrient_frac", 0.0f);
    ing.requires_pasteurization = tbl_bool(L, 1, "requires_pasteurization", false);
    ing.d_ref_sec               = tbl_float(L, 1, "d_ref_sec", 32.0f);
    ing.t_ref_c                 = tbl_float(L, 1, "t_ref_c", 60.0f);
    ing.z_val_c                 = tbl_float(L, 1, "z_val_c", 5.5f);

    ing.fiber_anisotropy = tbl_float(L, 1, "fiber_anisotropy", 1.0f);
    char shape_buf[24];
    tbl_str(L, 1, "shape", shape_buf, sizeof(shape_buf), "slab");
    ing.default_shape = parse_shape(shape_buf);
    ing.default_lx_mm = tbl_float(L, 1, "lx_mm", 80.0f);
    ing.default_ly_mm = tbl_float(L, 1, "ly_mm", 60.0f);
    ing.default_lz_mm = tbl_float(L, 1, "lz_mm", 25.0f);
    ing.diet_flags    = parse_diet_flags(L, 1);
    ing.role_flags    = parse_role_flags(L, 1);

    int existing = fd_db_find_ingredient(h->db, ing.id);
    if (existing >= 0) {
        h->db->ingredients[existing] = ing;
    } else if (h->db->ingredient_count < FD_MAX_INGREDIENTS) {
        h->db->ingredients[h->db->ingredient_count++] = ing;
    } else {
        return luaL_error(L, "ingredient capacity full");
    }
    return 0;
}

static int l_add_technique(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    fd_lua_t *h = get_host(L);
    fd_technique_t t;
    memset(&t, 0, sizeof(t));
    tbl_str(L, 1, "id", t.id, sizeof(t.id), "");
    if (t.id[0] == '\0') return luaL_error(L, "technique missing id");
    tbl_str(L, 1, "name", t.name, sizeof(t.name), t.id);
    t.h_bottom         = tbl_float(L, 1, "h_bottom", 250.0f);
    t.h_top            = tbl_float(L, 1, "h_top", 15.0f);
    t.h_side           = tbl_float(L, 1, "h_side", 20.0f);
    t.emissivity_rad   = tbl_float(L, 1, "emissivity_rad", 0.0f);
    t.ambient_humidity = tbl_float(L, 1, "ambient_humidity", 0.30f);
    t.default_temp_c   = tbl_float(L, 1, "default_temp_c", 180.0f);
    t.min_temp_c       = tbl_float(L, 1, "min_temp_c", 20.0f);
    t.max_temp_c       = tbl_float(L, 1, "max_temp_c", 260.0f);
    t.oil_uptake_rate  = tbl_float(L, 1, "oil_uptake_rate", 0.0f);
    t.shear_rate       = tbl_float(L, 1, "shear_rate", 0.0f);
    t.immersed_water   = tbl_bool(L, 1, "immersed_water", false);
    t.immersed_oil     = tbl_bool(L, 1, "immersed_oil", false);
    t.sealed_pouch     = tbl_bool(L, 1, "sealed_pouch", false);

    int existing = fd_db_find_technique(h->db, t.id);
    if (existing >= 0) {
        h->db->techniques[existing] = t;
    } else if (h->db->technique_count < FD_MAX_TECHNIQUES) {
        h->db->techniques[h->db->technique_count++] = t;
    } else {
        return luaL_error(L, "technique capacity full");
    }
    return 0;
}

static int l_add_recipe(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    fd_lua_t *h = get_host(L);
    fd_recipe_t r;
    memset(&r, 0, sizeof(r));
    tbl_str(L, 1, "id", r.id, sizeof(r.id), "");
    if (r.id[0] == '\0') return luaL_error(L, "recipe missing id");
    tbl_str(L, 1, "name", r.name, sizeof(r.name), r.id);
    tbl_str(L, 1, "category", r.category, sizeof(r.category), "classic");
    tbl_str(L, 1, "notes", r.notes, sizeof(r.notes), "");
    char sbuf[24];
    tbl_str(L, 1, "shape", sbuf, sizeof(sbuf), "slab");
    r.shape = parse_shape(sbuf);
    r.lx_mm = tbl_float(L, 1, "lx_mm", 90.0f);
    r.ly_mm = tbl_float(L, 1, "ly_mm", 70.0f);
    r.lz_mm = tbl_float(L, 1, "lz_mm", 28.0f);
    r.chill_after = tbl_bool(L, 1, "chill_after", false);

    lua_getfield(L, 1, "items");
    if (lua_istable(L, -1)) {
        int n = (int)luaL_len(L, -1);
        for (int i = 1; i <= n && r.item_count < FD_MAX_RECIPE_ITEMS; i++) {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1)) {
                fd_recipe_item_t *it = &r.items[r.item_count++];
                tbl_str(L, -1, "id", it->ingredient_id, sizeof(it->ingredient_id), "");
                it->mass_g = tbl_float(L, -1, "mass_g", 50.0f);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "steps");
    if (lua_istable(L, -1)) {
        int n = (int)luaL_len(L, -1);
        for (int i = 1; i <= n && r.step_count < FD_MAX_STEPS; i++) {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1)) {
                fd_recipe_step_t *st = &r.steps[r.step_count++];
                tbl_str(L, -1, "technique", st->technique_id, sizeof(st->technique_id), "pan_sear");
                st->temp_c       = tbl_float(L, -1, "temp_c", 180.0f);
                st->duration_sec = tbl_float(L, -1, "duration_sec", 120.0f);
                st->flip_midway  = tbl_bool(L, -1, "flip_midway", false);
                st->lid_closed   = tbl_bool(L, -1, "lid_closed", false);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    int existing = fd_db_find_recipe(h->db, r.id);
    if (existing >= 0) {
        h->db->recipes[existing] = r;
    } else if (h->db->recipe_count < FD_MAX_RECIPES) {
        h->db->recipes[h->db->recipe_count++] = r;
    } else {
        return luaL_error(L, "recipe capacity full");
    }
    return 0;
}

static int l_register_mod(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    fd_lua_t *h = get_host(L);
    lua_getfield(L, LUA_REGISTRYINDEX, FD_MOD_LIST_KEY);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, FD_MOD_LIST_KEY);
    }
    int len = (int)luaL_len(L, -1);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, len + 1);
    lua_pop(L, 1);
    h->mod_count = len + 1;

    /* Run optional on_init immediately */
    lua_getfield(L, 1, "on_init");
    if (lua_isfunction(L, -1)) {
        lua_call(L, 0, 0);
    } else {
        lua_pop(L, 1);
    }
    return 0;
}

static void init_default_reactions(fd_reactions_t *r) {
    r->maillard   = (fd_arrhenius_t){ 1.25e11, 102000.0 };
    r->caramel    = (fd_arrhenius_t){ 1.03e13, 124000.0 };
    r->pyrolysis  = (fd_arrhenius_t){ 6.00e14, 154000.0 };
    r->myosin     = (fd_arrhenius_t){ 2.00e28, 190000.0 };
    r->actin      = (fd_arrhenius_t){ 1.84e31, 218000.0 };
    r->albumen    = (fd_arrhenius_t){ 1.60e33, 236000.0 };
    r->collagen   = (fd_arrhenius_t){ 4.10e12, 112000.0 };
    r->starch_gel = (fd_arrhenius_t){ 8.50e19, 145000.0 };
    r->retrogradation_rate  = 3.5e-5f;
    r->target_pasteur_log10 = 6.5f;
    r->umami_synergy_gamma  = 0.08f;
}

bool fd_lua_init(fd_lua_t *lua, const fd_vfs_t *vfs, fd_database_t *db) {
    if (!lua || !vfs || !db) return false;
    memset(lua, 0, sizeof(*lua));
    lua->vfs = vfs;
    lua->db  = db;
    lua->mem_limit = 16u * 1024u * 1024u;
    init_default_reactions(&db->reactions);

    lua_State *L = lua_newstate(sandbox_alloc, lua);
    if (!L) return false;
    lua->L = L;

    lua_pushlightuserdata(L, lua);
    lua_setfield(L, LUA_REGISTRYINDEX, FD_LUA_REG_KEY);

    /* Open ONLY safe pure computation libraries — no os, io, package, debug */
    luaL_requiref(L, LUA_GNAME, luaopen_base, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1); lua_pop(L, 1);

    /* Strip filesystem-touching globals from base */
    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "loadfile");

    static const luaL_Reg api[] = {
        { "include",       l_include },
        { "set_reactions", l_set_reactions },
        { "add_ingredient",l_add_ingredient },
        { "add_technique", l_add_technique },
        { "add_recipe",    l_add_recipe },
        { "register_mod",  l_register_mod },
        { NULL, NULL }
    };
    luaL_newlib(L, api);
    lua_setglobal(L, "fatduck");

    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, FD_MOD_LIST_KEY);
    return true;
}

void fd_lua_free(fd_lua_t *lua) {
    if (!lua) return;
    if (lua->L) {
        lua_close(lua->L);
        lua->L = NULL;
    }
}

bool fd_lua_exec_str(fd_lua_t *lua, const char *chunk_name, const char *code, int max_instr) {
    if (!lua || !lua->L || !code) return false;
    lua_State *L = lua->L;
    lua->instr_budget = (max_instr > 0) ? max_instr : 500000;
    lua_sethook(L, instr_limit_hook, LUA_MASKCOUNT, 1000);

    int st = luaL_loadbufferx(L, code, strlen(code), chunk_name ? chunk_name : "@chunk", "t");
    if (st == LUA_OK) {
        st = lua_pcall(L, 0, 0, 0);
    }
    lua_sethook(L, NULL, 0, 0);
    if (st != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        snprintf(lua->last_error, sizeof(lua->last_error), "%s", err ? err : "lua error");
        lua_pop(L, 1);
        return false;
    }
    lua->last_error[0] = '\0';
    return true;
}

bool fd_lua_exec_file(fd_lua_t *lua, const char *rel_path, int max_instr) {
    if (!lua || !lua->vfs || !fd_vfs_valid_relpath(rel_path)) {
        if (lua) snprintf(lua->last_error, sizeof(lua->last_error), "invalid path");
        return false;
    }
    uint8_t *buf = NULL;
    size_t sz = 0;
    if (!fd_vfs_read(lua->vfs, rel_path, &buf, &sz)) {
        snprintf(lua->last_error, sizeof(lua->last_error), "missing file: %s", rel_path);
        return false;
    }
    char cname[160];
    snprintf(cname, sizeof(cname), "@%s", rel_path);
    bool ok = fd_lua_exec_str(lua, cname, (const char *)buf, max_instr);
    free(buf);
    return ok;
}

bool fd_lua_reload_all(fd_lua_t *lua) {
    if (!lua || !lua->db) return false;
    const fd_vfs_t *vfs = lua->vfs;
    fd_database_t *db = lua->db;
    fd_lua_free(lua);
    memset(db, 0, sizeof(*db));
    if (!fd_lua_init(lua, vfs, db)) return false;
    return fd_lua_exec_file(lua, "scripts/init.lua", 2000000);
}

void fd_lua_call_on_step(fd_lua_t *lua, double elapsed_sec, float core_temp_c, float surf_temp_c) {
    if (!lua || !lua->L || lua->mod_count <= 0) return;
    lua_State *L = lua->L;
    lua->instr_budget = 50000;
    lua_sethook(L, instr_limit_hook, LUA_MASKCOUNT, 1000);

    lua_getfield(L, LUA_REGISTRYINDEX, FD_MOD_LIST_KEY);
    if (lua_istable(L, -1)) {
        int n = (int)luaL_len(L, -1);
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, -1, i);
            lua_getfield(L, -1, "on_step_post");
            if (lua_isfunction(L, -1)) {
                lua_newtable(L);
                lua_pushnumber(L, elapsed_sec); lua_setfield(L, -2, "elapsed_sec");
                lua_pushnumber(L, core_temp_c); lua_setfield(L, -2, "core_temp_c");
                lua_pushnumber(L, surf_temp_c); lua_setfield(L, -2, "surf_temp_c");
                if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                    lua_pop(L, 1);
                }
            } else {
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    lua_sethook(L, NULL, 0, 0);
}

float fd_lua_call_on_score(fd_lua_t *lua, const fd_metrics_t *m) {
    if (!lua || !lua->L || !m || lua->mod_count <= 0) return 0.0f;
    lua_State *L = lua->L;
    float bonus = 0.0f;
    lua->instr_budget = 50000;
    lua_sethook(L, instr_limit_hook, LUA_MASKCOUNT, 1000);

    lua_getfield(L, LUA_REGISTRYINDEX, FD_MOD_LIST_KEY);
    if (lua_istable(L, -1)) {
        int n = (int)luaL_len(L, -1);
        for (int i = 1; i <= n; i++) {
            lua_rawgeti(L, -1, i);
            lua_getfield(L, -1, "on_score_dish");
            if (lua_isfunction(L, -1)) {
                lua_newtable(L);
                lua_pushnumber(L, m->overall_score); lua_setfield(L, -2, "overall_score");
                lua_pushnumber(L, m->umami);         lua_setfield(L, -2, "umami");
                lua_pushnumber(L, m->crispness);     lua_setfield(L, -2, "crispness");
                lua_pushnumber(L, m->tenderness);    lua_setfield(L, -2, "tenderness");
                lua_pushnumber(L, m->estimated_gi);  lua_setfield(L, -2, "estimated_gi");
                if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
                    if (lua_isnumber(L, -1)) bonus += (float)lua_tonumber(L, -1);
                    lua_pop(L, 1);
                } else {
                    lua_pop(L, 1);
                }
            } else {
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    lua_sethook(L, NULL, 0, 0);
    return bonus;
}
