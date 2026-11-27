#ifndef FD_LUA_H
#define FD_LUA_H

#include "fd_types.h"
#include "fd_vfs.h"

typedef struct lua_State lua_State;

typedef struct {
    lua_State     *L;
    const fd_vfs_t *vfs;
    fd_database_t *db;
    size_t         mem_used;
    size_t         mem_limit;
    int            instr_budget;
    char           last_error[256];
    int            mod_count;
} fd_lua_t;

bool fd_lua_init(fd_lua_t *lua, const fd_vfs_t *vfs, fd_database_t *db);
void fd_lua_free(fd_lua_t *lua);

/* Execute a single Lua buffer or VFS script under instruction & memory sandbox */
bool fd_lua_exec_str(fd_lua_t *lua, const char *chunk_name, const char *code, int max_instr);
bool fd_lua_exec_file(fd_lua_t *lua, const char *rel_path, int max_instr);

/* Reload entire culinary knowledge base + mods from scripts/init.lua */
bool fd_lua_reload_all(fd_lua_t *lua);

/* Invoke registered mod hooks */
void  fd_lua_call_on_step(fd_lua_t *lua, double elapsed_sec, float core_temp_c, float surf_temp_c);
float fd_lua_call_on_score(fd_lua_t *lua, const fd_metrics_t *m);

/* Fast registry lookups */
int fd_db_find_ingredient(const fd_database_t *db, const char *id);
int fd_db_find_technique(const fd_database_t *db, const char *id);
int fd_db_find_recipe(const fd_database_t *db, const char *id);

/* Load a flat key->string Lua table (or table.strings) from VFS inside the sandbox */
typedef void (*fd_lua_kv_cb)(const char *key, const char *val, void *ud);
bool fd_lua_load_string_table(fd_lua_t *lua, const char *rel_path, fd_lua_kv_cb cb, void *ud);

#endif /* FD_LUA_H */
