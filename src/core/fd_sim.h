#ifndef FD_SIM_H
#define FD_SIM_H

#include "fd_lua.h"
#include "fd_types.h"

/* Single-vessel initialization & PDE stepping */
bool fd_sim_init_dish(fd_sim_state_t *sim, const fd_database_t *db,
                      const fd_recipe_item_t *items, int item_count,
                      fd_shape_type_t shape, float lx_mm, float ly_mm, float lz_mm,
                      float init_temp_c, bool coarse_mode);

void fd_sim_free(fd_sim_state_t *sim);

bool fd_sim_set_technique(fd_sim_state_t *sim, const fd_database_t *db,
                          const char *tech_id, float temp_c, bool lid_closed);

void fd_sim_step(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua, float dt_sec);

/* Staged ingredient additions, mechanical movement actions, and enthalpy-conserving vessel transfers */
bool fd_sim_add_item(fd_sim_state_t *sim, const fd_database_t *db,
                     const fd_recipe_item_t *item, fd_shape_type_t fallback_shape,
                     float lx_mm, float ly_mm, float lz_mm,
                     float item_temp_c, bool coarse_mode);

bool fd_sim_apply_action(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua,
                         fd_step_action_t action, float agitation,
                         float cut_lx_mm, float cut_ly_mm, float cut_lz_mm,
                         fd_shape_type_t cut_shape, bool has_cut_shape,
                         float temp_c, float duration_sec, bool coarse_mode);

bool fd_sim_combine_vessels(fd_sim_state_t *dst, fd_sim_state_t *src,
                            const fd_database_t *db, fd_lua_t *lua,
                            bool gentle_fold, bool coarse_mode);

/* Multi-vessel kitchen orchestration & detailed chef-executable step formatter */
bool fd_kitchen_init_recipe(fd_kitchen_t *k, const fd_database_t *db,
                            const fd_recipe_t *recipe, bool coarse_mode);
void fd_kitchen_free(fd_kitchen_t *k);
void fd_kitchen_step_live(fd_kitchen_t *k, const fd_database_t *db, fd_lua_t *lua, float dt_sec);
bool fd_kitchen_exec_next_step(fd_kitchen_t *k, const fd_database_t *db, fd_lua_t *lua);
bool fd_kitchen_run_recipe(fd_kitchen_t *k, const fd_database_t *db, fd_lua_t *lua,
                           const fd_recipe_t *recipe, bool coarse_mode);

/* Delegates to fd_kitchen_run_recipe and outputs the final plated dish into *sim */
bool fd_sim_run_recipe(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua,
                       const fd_recipe_t *recipe, bool coarse_mode);

/* Format human-executable chef instruction and first-principles rationale for step_idx */
void fd_recipe_describe_step(const fd_recipe_t *r, const fd_database_t *db, int step_idx,
                             char *out_instr, size_t instr_cap,
                             char *out_why, size_t why_cap);

#endif /* FD_SIM_H */
