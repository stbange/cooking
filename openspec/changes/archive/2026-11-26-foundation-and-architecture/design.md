# Design

## Context

FatDuck runs across Windows, Linux, macOS, and Emscripten WebAssembly. Embedding assets violates the extensibility requirement, while unrestricted Lua scripts would create security and hang risks when users edit mods in the browser or share scripts.

## Goals / Non-Goals

**Goals:**
- Single CMake build configuration supporting native desktop targets, coverage builds (`--coverage`), and Emscripten WASM (`-sUSE_SDL=3` / WebGL2).
- Simple, zero-dependency VFS (`fd_vfs`) with path-traversal protection (`..` rejection), disk loading on Desktop, pre-populated memory cache from HTTP `fetch` on Web, and an override layer for live Lua editing.
- Sandboxed Lua 5.4 host (`fd_lua`) using vendored ANSI C Lua 5.4.7 sources with stripped OS/IO libraries and `lua_sethook` instruction limits.
- Deterministic Python generator (`tools/gen_assets.py`) producing external CC BY 4.0 `.png` atlases, bitmap font, synthesized `.wav` kitchen audio, GLSL shaders, and `assets/manifest.json`.

**Non-Goals:**
- Complex third-party archive formats (`.zip`/`.pak`) or third-party JSON/FreeType/audio mixer libraries.

## Decisions

- **Vendored ANSI C Lua 5.4.7**: Compiles cleanly in <2 seconds across GCC, Clang, MSVC, and `emcc` without host package discrepancies.
- **Flat JSON Array Manifest**: `assets/manifest.json` uses a simple `["path/a", "path/b"]` array parsed in ~35 lines of C with strict bounds and `..` rejection—zero JSON library needed.
- **Bitmap Font + Sprite PNGs via `stb_image.h`**: Avoids `SDL_ttf`/FreeType binary bloat while keeping fonts and sprites editable in `assets/fonts/` and `assets/graphics/`.

## Risks / Trade-offs

- **Risk**: Relative working directories differ when launching binaries from Finder/Explorer vs. CLI.
  - **Mitigation**: Probe both `SDL_GetBasePath()` + `assets/` and `./assets/` (plus `FATDUCK_ASSETS_DIR` env override for tests).
