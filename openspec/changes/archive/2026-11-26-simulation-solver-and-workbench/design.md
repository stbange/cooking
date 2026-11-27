# Design

## Context

Users prioritized accurate 3D culinary physics while saving GPU/UI processing power, alongside cross-platform C11/SDL3/OpenGL/WebAssembly portability, unbaked assets, ≥95% core test coverage, and `main`-only GitHub Pages deployment.

## Goals / Non-Goals

**Goals:**
- Linear-time unconditionally stable 3D ADI heat and moisture solver supporting 9 culinary 3D shape/tissue archetypes and collapsing to `1x1x1` on homogeneous liquids.
- Frame-sliced `8^3 -> full 3D` evolutionary recipe search so single-threaded WASM stays smooth at 60 FPS without requiring `SharedArrayBuffer` headers on GitHub Pages.
- Renderer-agnostic immediate-mode 2D UI (`fd_ui`, `fd_app`) inside `src/core/` so Desktop and Mobile Touch journeys are 100% deterministic and unit/replay-testable headlessly.

**Non-Goals:**
- 3D fluid CFD or 3D mesh/camera rendering overhead.

## Decisions

- **3-Pass Thomas ADI**: Eliminates explicit CFL time-step collapse on thin skins/fillets while capturing 3-axis corner charring and asymmetric cold spots.
- **Single-Atlas White Pixel Batching**: Storing a `2x2` solid white texel at `(0,0)` of `fatduck_font.png` lets panels, plots, buttons, and glyph quads share one VBO/shader draw batch.
- **GitHub Actions `deploy-pages`**: Publishes `build-web` directly from `main` via Pages artifacts so no `gh-pages` branch is ever created.

## Risks / Trade-offs

- **Trade-off**: Operator-split ADI has mild $O(\Delta t)$ splitting error on extreme time steps, mitigated by capping internal sub-steps to `<= 2.5s`.
