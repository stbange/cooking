# recipe-proposal-and-invention Specification

## Purpose

Proposes adapted classic recipes and invents new physically valid recipes from user dietary, nutritional, time, and sensory constraints without external APIs.

## Requirements

### Requirement: Classic recipe matching with chemical substitution
The solver SHALL filter classic recipes against user constraints, replace restricted ingredients using functional chemical cosine similarity, and verify candidates via 3D simulation.

#### Scenario: Proposing vegan low-GI recipes
- **WHEN** `fd_solver_propose_classics` runs with `FD_DIET_VEGAN` and `max_gi` constraints
- **THEN** it returns ranked, 3D-simulated recipes satisfying the dietary flags and pasteurization safety

### Requirement: Multi-fidelity 3D evolutionary recipe invention
The solver SHALL evolve novel ingredient masses, 3D cut thicknesses, and cooking schedules on coarse `8x8x8` 3D grids in frame-sliced steps and verify top candidates on a full-resolution 3D grid.

#### Scenario: Inventing a safe high-protein dish
- **WHEN** `fd_inventor_start` and `fd_inventor_step` complete for a `min_protein_g` constraint
- **THEN** `inv.done` becomes true and `inv.best` holds a pasteurized-safe full-3D-verified recipe
