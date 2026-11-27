#ifndef FD_CHEMISTRY_H
#define FD_CHEMISTRY_H

#include "fd_types.h"

double fd_arrhenius_rate(fd_arrhenius_t arr, float temp_c);
float  fd_compute_ph(float acid_mol, float base_mol, float raw_weight_g, float base_ph);

void fd_chemistry_step(fd_sim_state_t *sim, const fd_database_t *db, float dt_sec);

#endif /* FD_CHEMISTRY_H */
