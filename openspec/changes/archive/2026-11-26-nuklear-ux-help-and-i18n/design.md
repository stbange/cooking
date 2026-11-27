## Context

FatDuck runs a 3D volumetric culinary simulator (`fatduck_core`, C11) rendered via a thin SDL3 + OpenGL 3.3 Core / WebGL2 shell on Windows, Linux, macOS, and Emscripten Web. All assets live unbaked inside `./assets/`. Core line coverage must stay `>= 95%`.

## Goals / Non-Goals

**Goals:**
- Integrate **Nuklear v4.13.4** (`NK_INCLUDE_VERTEX_BUFFER_OUTPUT`, `NK_INCLUDE_FONT_BAKING`) purely inside `fatduck_core` (via a separate `nuklear` static lib) so headless tests exercise the real UI layout and widgets.
- Support the **Top 20 spoken languages** with external `./assets/locales/<code>.lua` files and external subsetted **SIL OFL 1.1 Noto** fonts in `./assets/fonts/` (total < 1 MB).
- Render crisp, modern UI cards, rounded buttons, dropdown selectors, live 3D slice color legends, crosshair markers, telemetry bars/curves, contextual tooltips, interactive 6-step tutorial, and tabbed help modal across Desktop and Mobile Touch viewports.

**Non-Goals:**
- Full HarfBuzz/ICU complex Indic conjunct shaping (documented shortcut: pre-base matra reordering is handled cleanly; full OpenType GSUB conjuncts are deferred).
- Native OS system dialogs or baked-in font binaries.

## Decisions

1. **Separate `nuklear` static library target**: Compiles `third_party/nuklear/nuklear_impl.c` once, isolating third-party warnings and excluding unused Nuklear widgets from `src/core/` coverage metrics.
2. **Dynamic active-locale glyph atlas**: `fd_i18n_collect_runes` gathers the exact Unicode codepoints present in the active locale + ASCII/units, and `fd_ui_rebuild_fonts` bakes only those runes from `NotoSans-UI.ttf` + the active script's Noto font into a compact (`<= 1024x1024`) RGBA atlas with 2× oversampling and `GL_LINEAR` filtering.
3. **Arabic & Urdu RTL shaping in C**: `fd_i18n_shape_text` maps Arabic/Urdu letters to Unicode Arabic Presentation Forms-A/B (`U+FB50..U+FEFF`, including `لا` ligatures) and reverses RTL visual runs while preserving embedded LTR tokens (numbers, units, `FatDuck`, `XY/XZ/YZ`).
4. **Lua-backed locales**: Each `assets/locales/<code>.lua` returns a plain Lua table executed in the existing sandboxed Lua 5.4 host with `en` fallback, making translations directly moddable alongside scripts.
5. **Non-blocking onboarding**: Interactive 6-step tutorial opens on demand (`Tutorial` button or subtle dismissible welcome tip) so users and E2E tests are never blocked by mandatory popups.
