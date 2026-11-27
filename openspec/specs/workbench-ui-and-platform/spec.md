# workbench-ui-and-platform Specification

## Purpose

Renders the lightweight 2D orthographic slice workbench across Desktop and Mobile Web touch devices and enforces ≥95% core test coverage plus `main`-only GitHub Pages deployment.

## Requirements

### Requirement: 2D orthographic slice inspection and cold-spot snapping
The workbench SHALL extract `XY`, `XZ`, and `YZ` 2D cross-sections from the 3D voxel grid and provide one-tap snapping to the 3D cold-spot and peak-char voxels.

#### Scenario: Snapping slice depth to 3D cold spot
- **WHEN** the user activates `ColdSpot` in the slice inspector
- **THEN** `slice_norm` updates to intersect `(cold_x, cold_y, cold_z)` on the active plane

### Requirement: Automatic mobile touch layout switching
The Web and native UI SHALL switch to touch-friendly navigation (`>=48px` targets and bottom thumb bar) upon touch input or mobile viewport dimensions.

#### Scenario: Opening Web build on a mobile touch viewport
- **WHEN** the Emscripten build loads in a `390x844` touch-enabled browser context
- **THEN** `touch_mode` activates automatically and touch taps navigate tabs

### Requirement: Coverage and automated test gates
The repository SHALL maintain ≥95% line coverage on `src/core/`, automated headless game replay tests, Playwright WebGL2/touch E2E tests, and `main`-only GitHub Actions deployment to GitHub Pages.

#### Scenario: Running coverage verification
- **WHEN** `scripts/check_coverage.sh` runs after `ctest`
- **THEN** measured line coverage on `src/core/` is at least `95.0%`

### Requirement: Nuklear modern UI rendering with external TrueType/OpenType fonts
The application SHALL render its immediate-mode UI using Nuklear (`NK_INCLUDE_VERTEX_BUFFER_OUTPUT`) styled with a modern dark-slate/saffron palette, rounded controls, scissor clipping, smooth color-legend cross-sections, and external unbaked Noto fonts loaded from `./assets/fonts/`.

#### Scenario: Rendering workbench with Nuklear indexed draw lists
- **WHEN** `fd_app_update_and_render` runs a frame on Desktop or Mobile Touch
- **THEN** Nuklear generates non-empty indexed vertex/element buffers and draw commands using the dynamically baked font atlas texture
