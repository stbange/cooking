#ifndef FD_SOLVER_H
#define FD_SOLVER_H

#include "fd_sim.h"
#include "fd_types.h"

#define FD_MAX_PROPOSALS 8
#define FD_INV_POP_SIZE  6

typedef struct {
    fd_recipe_t  recipe;
    fd_metrics_t metrics;
    float        fitness;
    int          substitutions;
    char         rationale[192];
} fd_proposal_t;

typedef struct {
    fd_constraints_t constraints;
    uint32_t         rng;
    bool             active;
    bool             done;
    int              evals_done;
    int              evals_target;

    int              pop_count;
    fd_recipe_t      pop[FD_INV_POP_SIZE];
    fd_metrics_t     pop_metrics[FD_INV_POP_SIZE];
    float            pop_fitness[FD_INV_POP_SIZE];

    fd_proposal_t    best;
} fd_inventor_t;

void  fd_constraints_init_default(fd_constraints_t *c);
float fd_ingredient_similarity(const fd_ingredient_t *a, const fd_ingredient_t *b);
float fd_evaluate_fitness(const fd_metrics_t *m, const fd_recipe_t *r, const fd_constraints_t *c);

/* Propose & adapt classic recipes matching constraints (with functional chemical substitution) */
int fd_solver_propose_classics(const fd_database_t *db, fd_lua_t *lua,
                               const fd_constraints_t *c,
                               fd_proposal_t *out_proposals, int max_out);

/* Frame-sliced multi-fidelity 3D evolutionary recipe invention (8x8x8 screen -> 24x24x24 verify) */
bool  fd_inventor_start(fd_inventor_t *inv, const fd_database_t *db, const fd_constraints_t *c);
bool  fd_inventor_step(fd_inventor_t *inv, const fd_database_t *db, fd_lua_t *lua, int budget_evals);
float fd_inventor_progress(const fd_inventor_t *inv);

#endif /* FD_SOLVER_H */
