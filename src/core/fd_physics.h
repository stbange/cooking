#ifndef FD_PHYSICS_H
#define FD_PHYSICS_H

#include "fd_types.h"

/* Choi-Okos (1986) temperature-dependent constituent properties */
void fd_choi_okos(float T_c, float x_water, float x_ice, float x_prot,
                  float x_fat, float x_carb, float x_fiber, float x_ash, float porosity,
                  float *out_rho, float *out_cp_eff, float *out_k);

/* Water activity from local moisture, dissolved salt, and sugar */
float fd_compute_water_activity(float water_frac, float salt_frac, float sugar_frac);

/* Advance 3D heat, phase change, surface evaporation, moisture diffusion, and emulsion rheology */
void fd_physics_step(fd_sim_state_t *sim, const fd_database_t *db, float dt_sec);

#endif /* FD_PHYSICS_H */
