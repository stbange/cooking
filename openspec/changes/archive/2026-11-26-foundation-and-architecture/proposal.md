# Proposal

## Why

FatDuck requires a cross-platform C11, SDL3, OpenGL, and Lua 5.4 foundation where all assets (graphics, fonts, sounds, shaders, and Lua scripts) remain unbaked in `./assets/` beside the binary on Desktop and are fetched dynamically on Web, with ≥95% core test coverage.

## What Changes

- Establish the cross-platform CMake build (`Linux`, `Windows`, `macOS`, `Emscripten`, and `FATDUCK_COVERAGE`) compiling `libfatduck_core`, vendored Lua 5.4, and SDL3.
- Add the external Virtual Filesystem (`fd_vfs`) supporting relative sibling `./assets/` files, `assets/manifest.json` indexing for Web HTTP fetching, and runtime/browser `localStorage` script overrides.
- Add the sandboxed Lua 5.4 loader (`fd_lua`) for declarative culinary data tables and safe mod hooks (`on_init`, `on_step_post`, `on_score_dish`).
- Add deterministic external asset generation (`tools/gen_assets.py`) outputting CC BY 4.0 PNG atlases, bitmap font, procedural kitchen WAV files, GLSL shaders, and `assets/manifest.json`.

## Capabilities

### New Capabilities
- `vfs-and-external-assets`: Unbaked external asset discovery, manifest parsing, and live script override storage across Desktop and Emscripten Web builds.
- `lua-data-and-modding`: Sandboxed Lua 5.4 execution for declarative ingredient, reaction, technique, and recipe tables plus extensible mod hooks.

### Modified Capabilities

## Impact

- Creates `CMakeLists.txt`, `src/core/fd_types.h`, `src/core/fd_vfs.c/.h`, `src/core/fd_lua.c/.h`, `tools/gen_assets.py`, `assets/`, and initial unit tests in `tests/unit/`.
