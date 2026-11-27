## ADDED Requirements

### Requirement: Detailed executable multi-station recipe generation
Both `fd_solver_propose_classics` and `fd_inventor_step` SHALL evaluate candidates through the multi-vessel kitchen simulator and expose complete human-executable step instructions (specifying vessel, ingredient gram masses, 3D cut dimensions in `mm`, mechanical/thermal action, temperature, duration, and doneness rationale).

#### Scenario: Inventing a multi-station executable recipe
- **WHEN** `fd_inventor_step` completes synthesis of a novel recipe
- **THEN** the resulting `fd_recipe_t` contains multi-step prep/cook/finish instructions readable via `fd_recipe_describe_step`
