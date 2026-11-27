#ifndef FD_NUTRITION_H
#define FD_NUTRITION_H

#include "fd_types.h"

/* Compute cooked mass-balance macros, starch-state eGI/eGL, and protein bioavailability */
void fd_nutrition_update(fd_sim_state_t *sim);

#endif /* FD_NUTRITION_H */
