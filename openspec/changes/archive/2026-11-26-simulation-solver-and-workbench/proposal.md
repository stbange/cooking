# Proposal

## Why

To recreate classic recipes and invent new physically valid dishes, FatDuck needs a 3D volumetric simulation of heat/mass transfer, chemical reaction kinetics, microbial/fermentation/sensory biology, and glycemic/digestibility nutrition, coupled to an offline evolutionary solver, a lightweight 2D orthographic slice UI with mobile touch controls, and multi-platform CI/Pages deployment.

## What Changes

- Implement the 3D voxel simulation engine (`fd_geometry`, `fd_physics`, `fd_chemistry`, `fd_biology`, `fd_nutrition`, `fd_sim`) with 3-pass Thomas ADI diffusion, Choi–Okos properties, Arrhenius reaction networks, Bigelow $D/z$ pasteurization, and cooked glycemic/bioavailability nutrition.
- Populate the external Lua dataset (`assets/scripts/`) with 106+ USDA-grounded ingredients, 15 cooking techniques, 32+ classic recipes, and a working mod example.
- Implement `fd_solver` for chemical-similarity classic recipe adaptation and frame-sliced multi-fidelity 3D evolutionary recipe invention (`8x8x8` -> full 3D grid).
- Implement `fd_ui`, `fd_app`, and `src/platform/` (`main.c`, `gl_renderer.c`, `audio_sdl.c`, `web/shell.html`) with automatic mobile touch switching and GitHub Actions CI + GitHub Pages deployment on `main`.

## Capabilities

### New Capabilities
- `simulation-3d-engine`: 3D voxel heat, moisture, phase-change, Arrhenius chemistry, pasteurization/fermentation/sensory biology, and glycemic/bioavailability nutrition simulation.
- `recipe-proposal-and-invention`: Offline constraint matching, functional ingredient substitution, and multi-fidelity 3D evolutionary recipe synthesis.
- `workbench-ui-and-platform`: 2D orthographic XY/XZ/YZ slice UI, automatic mobile touch adaptation, SDL3/OpenGL/WebGL2 shell, ≥95% core test coverage, and GitHub Pages deployment.

### Modified Capabilities

## Impact

- Adds `fd_geometry`, `fd_physics`, `fd_chemistry`, `fd_biology`, `fd_nutrition`, `fd_sim`, `fd_solver`, `fd_ui`, `fd_app`, `src/platform/*`, `assets/scripts/*`, `tests/*`, and `.github/workflows/ci-and-pages.yml`.
