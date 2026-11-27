#ifndef FD_BIOLOGY_H
#define FD_BIOLOGY_H

#include "fd_types.h"

/* Rosso cardinal temperature activity curve [0..1] */
float fd_cardinal_temp_activity(float T_c, float t_min, float t_opt, float t_max);

/* Step 3D cold-spot microbial lethality, yeast/LAB fermentation, enzymes, and sensory scoring */
void fd_biology_step(fd_sim_state_t *sim, const fd_database_t *db, float dt_sec);

#endif /* FD_BIOLOGY_H */
