# Tasks

## 1. Build & External Assets Pipeline

- [x] 1.1 Create `tools/gen_assets.py` generating CC BY 4.0 `kitchen_atlas.png`, `icons.png`, `fatduck_font.png`, `fatduck_font.lua`, synthesized kitchen `.wav` files, GLSL shaders, and `assets/manifest.json`
- [x] 1.2 Create `CMakeLists.txt` for `fatduck_core`, Lua 5.4, SDL3, native/Web targets, and `gcov` coverage instrumentation

## 2. Virtual Filesystem & Sandboxed Lua Host

- [x] 2.1 Define core POD data structures in `src/core/fd_types.h`
- [x] 2.2 Implement `src/core/fd_vfs.c` and `src/core/fd_vfs.h` with sibling folder discovery, safe manifest parsing, and in-memory script overrides
- [x] 2.3 Implement `src/core/fd_lua.c` and `src/core/fd_lua.h` with instruction-counted sandboxing, declarative table parsing, and mod hook callbacks
- [x] 2.4 Add unit tests `tests/unit/test_core_unit.c` achieving ≥95% line coverage on `fd_vfs` and `fd_lua`
