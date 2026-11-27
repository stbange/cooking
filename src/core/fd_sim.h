#ifndef FD_SIM_H
#define FD_SIM_H

#include "fd_lua.h"
#include "fd_types.h"

bool fd_sim_init_dish(fd_sim_state_t *sim, const fd_database_t *db,
                      const fd_recipe_item_t *items, int item_count,
                      fd_shape_type_t shape, float lx_mm, float ly_mm, float lz_mm,
                      float init_temp_c, bool coarse_mode);

void fd_sim_free(fd_sim_state_t *sim);

bool fd_sim_set_technique(fd_sim_state_t *sim, const fd_database_t *db,
                          const char *tech_id, float temp_c, bool lid_closed);

void fd_sim_step(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua, float dt_sec);

bool fd_sim_run_recipe(fd_sim_state_t *sim, const fd_database_t *db, fd_lua_t *lua,
                       const fd_recipe_t *recipe, bool coarse_mode);

#endif /* FD_SIM_H */
