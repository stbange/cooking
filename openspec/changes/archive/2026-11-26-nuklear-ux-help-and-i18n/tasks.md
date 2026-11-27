## 1. Nuklear & Build Integration
- [x] 1.1 Vendor Nuklear v4.13.4 (`third_party/nuklear/nuklear.h`, `LICENSE`, `nuklear_impl.c`) and add `nuklear` static library to `CMakeLists.txt`.
- [x] 1.2 Upgrade `gl_renderer.{h,c}` and `src/platform/main.c` for indexed `glDrawElements`, `glScissor` clipping, HiDPI scaling, dynamic Noto font atlas upload, scroll wheel, `F1` help hotkey, `SDL_OpenURL`, and Web `navigator.language` + `localStorage` language persistence.

## 2. Internationalization (Top 20 World Languages) & External Fonts
- [x] 2.1 Implement `src/core/fd_i18n.{h,c}` with 20-language registry, sandboxed Lua locale loader, English fallback, dynamic glyph range collection, Arabic/Urdu contextual shaping + RTL BiDi visual ordering, and Indic pre-base matra reordering.
- [x] 2.2 Author external `./assets/locales/<code>.lua` translation packs for all 20 languages (`en`, `zh`, `hi`, `es`, `fr`, `ar`, `bn`, `pt`, `ru`, `ur`, `id`, `de`, `ja`, `sw`, `mr`, `te`, `tr`, `ta`, `vi`, `ko`).
- [x] 2.3 Update `tools/gen_assets.py` to generate smooth anti-aliased culinary art (`kitchen_atlas.png`, `icons.png`), subset SIL OFL 1.1 Noto fonts into `./assets/fonts/`, include `OFL-1.1.txt`, update `assets/LICENSE-ART.md`, and regenerate `assets/manifest.json`.

## 3. Modern Nuklear UI/UX, Interactive Tutorial & Online Help
- [x] 3.1 Rewrite `src/core/fd_ui.{h,c}` on top of Nuklear with a modern dark-slate/saffron culinary theme, dynamic font rebuilding, and crisp canvas widgets (slice legend, crosshairs, thermal plot, metric bars).
- [x] 3.2 Redesign `src/core/fd_app.{h,c}` with self-explanatory Kitchen Lab, Classics, Invent, and Mod Lab views, recipe/technique selectors, contextual tooltips, 20-language picker, 6-step interactive guided tutorial, and 5-tab help modal with online `docs/HELP.md` link.
- [x] 3.3 Write `docs/HELP.md` and update `README.md`.

## 4. Verification, Coverage & Deployment
- [x] 4.1 Expand `tests/unit/test_core_unit.c`, `tests/game/test_game_replay.c`, and `tests/web/e2e.spec.mjs` to verify Nuklear rendering, 20-language loading/shaping, tutorial, help modal, and mobile touch navigation.
- [x] 4.2 Verify `ctest`, `>=95%` coverage via `scripts/check_coverage.sh`, headless OpenGL run, Emscripten + Playwright E2E, strict OpenSpec validation, and green GitHub Actions + GitHub Pages on `main`.
