## Why

The initial FatDuck workbench used a minimal 8×8 bitmap font and custom flat-quad UI (`fd_ui.c`). To deliver a modern, approachable, self-explanatory culinary laboratory suitable for global users across Desktop and Mobile Web, FatDuck is upgrading its UI layer to **Nuklear** (MIT / Public Domain, ANSI C single-header immediate-mode GUI) with TrueType Noto typography, guided onboarding, contextual online help, and 20-language internationalization.

## What Changes

- Vendor **Nuklear v4.13.4** (`third_party/nuklear/`) as a separate static library target with MIT/Public Domain licensing.
- Replace the custom retro bitmap UI with a modern **Nuklear** dark-slate & saffron-gold UI powered by external unbaked **Noto Sans (SIL OFL 1.1)** `.ttf`/`.otf` fonts in `./assets/fonts/` and anti-aliased culinary vessel graphics.
- Add **Internationalization (`fd_i18n`)** supporting the **Top 20 most spoken languages in the world** (`en`, `zh`, `hi`, `es`, `fr`, `ar`, `bn`, `pt`, `ru`, `ur`, `id`, `de`, `ja`, `sw`, `mr`, `te`, `tr`, `ta`, `vi`, `ko`) loaded externally from `./assets/locales/*.lua`, including Arabic/Urdu contextual shaping + RTL visual BiDi reordering, Indic pre-base vowel reordering, and dynamic per-language Nuklear font atlas baking.
- Add a **6-step Interactive Guided Tutorial**, a **5-tab In-App Help Modal** (`F1` / `? Help`), contextual tooltips, and an online documentation link (`docs/HELP.md`).
- Preserve 3D first-principles physics/chemistry/biology/nutrition accuracy, automatic Mobile Touch layout switching, external unbaked assets, ≥95% core line coverage, headless game replay tests, and `main`-only multi-platform CI + GitHub Pages deployment.

## Capabilities

### New Capabilities
- `i18n-and-help`: External Lua-driven 20-language localization with dynamic Noto font baking, Arabic/Urdu RTL shaping, interactive 6-step tutorial, and in-app/online help system.

### Modified Capabilities
- `workbench-ui-and-platform`: Upgrades the 2D workbench UI renderer from custom bitmap quads to Nuklear indexed vertex/scissor rendering with TrueType/OpenType typography, smooth heatmaps with legend & crosshairs, recipe/technique dropdowns, and updated Desktop/Mobile tests.

## Impact

- `third_party/nuklear/`, `CMakeLists.txt`
- `src/core/fd_i18n.{h,c}`, `src/core/fd_ui.{h,c}`, `src/core/fd_app.{h,c}`, `src/core/fd_lua.{h,c}`
- `src/platform/gl_renderer.{h,c}`, `src/platform/main.c`, `web/shell.html`
- `assets/locales/*.lua`, `assets/fonts/*`, `assets/graphics/*`, `assets/manifest.json`, `assets/LICENSE-ART.md`, `tools/gen_assets.py`
- `docs/HELP.md`, `README.md`, `tests/unit/test_core_unit.c`, `tests/game/test_game_replay.c`, `tests/web/e2e.spec.mjs`
