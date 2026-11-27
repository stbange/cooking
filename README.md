# FatDuck

🌐 **Live Web Version (Desktop & Mobile Touch): [https://stbange.github.io/cooking/](https://stbange.github.io/cooking/)**  
📖 **Online Help & Culinary Science Manual: [docs/HELP.md](docs/HELP.md)**

**FatDuck** is a cross-platform first-principles culinary simulator and recipe invention engine written in **C11**, **SDL3**, **OpenGL (GL 3.3 Core / WebGL2)**, **Nuklear**, and **Lua 5.4**.

Instead of treating recipes as static text, FatDuck simulates cooking on a **3D volumetric voxel grid** driven by mathematical, physical, chemical, biological, and nutritional laws—reproducing classic dishes and inventing brand-new, physically working recipes from your constraints.

---

## Key Capabilities

- **True 3D First-Principles Engine (`src/core/`)**:
  - **Physics**: Choi–Okos constituent thermal properties ($k(T), c_p(T), \rho(T)$), unconditionally stable 3-pass Thomas ADI 3D heat & moisture diffusion, enthalpy ice fusion & boiling desiccation, 6-face boundary fluxes (capturing 3-axis corner charring & asymmetric cut lag), and Krieger–Dougherty emulsion rheology.
  - **Chemistry**: Arrhenius kinetics for Maillard browning, caramelization, pyrolysis, 6-stage protein denaturation (myosin, sarcoplasmic, actin syneresis, yolk, albumen, whey/gluten), collagen-to-gelatin hydrolysis, starch gelatinization/retrogradation, $\text{pH}$ buffering, and lipid smoke point.
  - **Biology**: 3D cold-spot Bigelow $D/z$ microbial pasteurization, yeast & lactic-acid fermentation, amylase/protease enzyme curves, and 5-taste + umami synergy + volatile aroma + 3D texture psychophysics.
  - **Nutrition & Digestion**: Cooked mass-balance macros, starch-gelatinization & gastric-emptying Estimated Glycemic Index / Load ($\text{eGI}/\text{eGL}$), and protein bioavailability.
- **Modern Nuklear UI, Guided Tutorial & Online Help**:
  - Clean dark-slate & saffron-gold **Nuklear** (MIT / Public Domain) interface with smooth **Google Noto Sans (SIL OFL 1.1)** typography, recipe & technique dropdowns, live `XY`/`XZ`/`YZ` 3D cross-section heatmaps with color-scale legend & Cold-Spot/Peak-Crust crosshairs, contextual tooltips, a **6-step Interactive Guided Tour**, and a **5-tab In-App Help Modal (`F1`)** linked to [docs/HELP.md](docs/HELP.md).
  - Automatically switches to thumb-friendly touch controls (`>=48px` targets + bottom navigation) on mobile browsers and narrow viewports.
- **20-Language Internationalization (`assets/locales/*.lua`)**:
  - Instant runtime switching across the **Top 20 spoken world languages** (`en`, `zh`, `hi`, `es`, `fr`, `ar`, `bn`, `pt`, `ru`, `ur`, `id`, `de`, `ja`, `sw`, `mr`, `te`, `tr`, `ta`, `vi`, `ko`), with Arabic/Urdu contextual RTL shaping, dynamic active-script font atlas baking, Web browser locale auto-detection, and `localStorage` persistence.
- **Classic Recipe Proposal & De Novo Invention (`fd_solver`)**:
  - Matches and chemically substitutes across **33+ classic recipes**, or runs a deterministic frame-sliced $(\mu + \lambda)$ multi-fidelity 3D evolutionary search (`8×8×8` screening $\to$ full 3D verification) across **107+ USDA-grounded ingredients** and **15 cooking techniques** offline.
- **100% Unbaked External Assets & Lua Modding (`./assets/`)**:
  - Graphics, Noto fonts, WAV audio, GLSL shaders, Lua scripts, and all 20 locale packs live unbaked in `./assets/` beside the executable (and are fetched loose via `./assets/manifest.json` on GitHub Pages with `localStorage` live mod overrides).

---

## Building & Testing

### Native (Linux, macOS, Windows)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/fatduck
```

### Unit Tests, 3D Science Verification, Game Replays & ≥95% Coverage Gate
```bash
cmake -B build-cov -DFATDUCK_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-cov -j
ctest --test-dir build-cov --output-on-failure
./scripts/check_coverage.sh build-cov
```

### Web (Emscripten)
```bash
emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web -j
npx playwright test
```

---

## Extending, Translating & Modding FatDuck

Drop or edit files directly inside `assets/` next to the binary (no rebuild required; press `F5` to hot-reload):
- `assets/scripts/ingredients/*.lua` — Add or tune ingredients from macronutrient/chemical composition.
- `assets/scripts/techniques.lua` — Adjust 6-face heat transfer and shear parameters.
- `assets/scripts/recipes/*.lua` — Add multi-step benchmark recipes.
- `assets/scripts/mods/*.lua` — Register custom ingredients, recipes, and `on_init` / `on_step_post` / `on_score_dish` hooks.
- `assets/locales/*.lua` — Edit or extend any of the 20 language packs.

---

## Licenses

- **Code, Lua Scripts, Locales & Shaders**: [MIT License](LICENSE) (includes vendored Lua 5.4 MIT and Nuklear MIT/Public Domain).
- **Original Visual Art & Audio (`assets/graphics/`, `assets/sounds/`)**: [CC BY 4.0](assets/LICENSE-ART.md).
- **Bundled Noto Font Subsets (`assets/fonts/`)**: SIL Open Font License 1.1 ([assets/fonts/OFL-1.1.txt](assets/fonts/OFL-1.1.txt)).
