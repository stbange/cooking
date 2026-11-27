# FatDuck

🌐 **Live Web Version (Desktop & Mobile Touch): [https://stbange.github.io/cooking/](https://stbange.github.io/cooking/)**

**FatDuck** is a cross-platform first-principles culinary simulator and recipe invention engine written in **C11**, **SDL3**, **OpenGL (GL 3.3 Core / WebGL2)**, and **Lua 5.4**.

Instead of treating recipes as static text, FatDuck simulates cooking on a **3D volumetric voxel grid** driven by mathematical, physical, chemical, biological, and nutritional laws—reproducing classic dishes and inventing brand-new, physically working recipes from your constraints.

---

## Key Capabilities

- **True 3D First-Principles Engine (`src/core/`)**:
  - **Physics**: Choi–Okos constituent thermal properties ($k(T), c_p(T), \rho(T)$), unconditionally stable 3-pass Thomas ADI 3D heat & moisture diffusion, enthalpy ice fusion & boiling desiccation, 6-face boundary fluxes (capturing 3-axis corner charring & asymmetric cut lag), and Krieger–Dougherty emulsion rheology.
  - **Chemistry**: Arrhenius kinetics for Maillard browning, caramelization, pyrolysis, 6-stage protein denaturation (myosin, sarcoplasmic, actin syneresis, yolk, albumen, whey/gluten), collagen-to-gelatin hydrolysis, starch gelatinization/retrogradation, $\text{pH}$ buffering, and lipid smoke point.
  - **Biology**: 3D cold-spot Bigelow $D/z$ microbial pasteurization, yeast & lactic-acid fermentation, amylase/protease enzyme curves, and 5-taste + umami synergy + volatile aroma + 3D texture psychophysics.
  - **Nutrition & Digestion**: Cooked mass-balance macros, starch-gelatinization & gastric-emptying Estimated Glycemic Index / Load ($\text{eGI}/\text{eGL}$), and protein bioavailability.
- **Classic Recipe Proposal & De Novo Invention (`fd_solver`)**:
  - Matches and chemically substitutes across **32+ classic recipes**, or runs a deterministic frame-sliced $(\mu + \lambda)$ multi-fidelity 3D evolutionary search (`8×8×8` screening $\to$ `24×24×24` verification) across **105+ USDA-grounded ingredients** and **15 cooking techniques** to invent new working dishes offline.
- **Lightweight 2D Slice Workbench & Automatic Mobile Touch**:
  - Fast 2D orthographic `XY` / `XZ` / `YZ` cross-section heatmaps with one-tap `[Cold Spot]` and `[Peak Char]` snapping; automatically switches to thumb-friendly touch controls on mobile browsers.
- **100% Unbaked External Assets & Lua Modding (`./assets/`)**:
  - Graphics, fonts, WAV audio, GLSL shaders, and Lua scripts live in `./assets/` beside the executable (and are fetched loose via `./assets/manifest.json` on GitHub Pages with `localStorage` live mod overrides).

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

## Extending & Modding FatDuck

Drop or edit files directly inside `assets/` next to the binary (no rebuild required; press `F5` to hot-reload):
- `assets/scripts/ingredients/*.lua` — Add or tune ingredients from macronutrient/chemical composition.
- `assets/scripts/techniques.lua` — Adjust 6-face heat transfer and shear parameters.
- `assets/scripts/recipes/*.lua` — Add multi-step benchmark recipes.
- `assets/scripts/mods/*.lua` — Register custom ingredients, recipes, and `on_init` / `on_step_post` / `on_score_dish` hooks.

---

## Specifications & Engineering Discipline

- **OpenSpec**: Living specifications and archived change history live in [`openspec/`](./openspec/).
- **Ponytail**: Built under [`AGENTS.md`](./AGENTS.md) ([DietrichGebert/ponytail](https://github.com/dietrichgebert/ponytail)).

---

## License

- **Code & Scripts**: [MIT License](./LICENSE) — Copyright (c) 2026 Stephan Bange.
- **Art, Fonts & Audio (`assets/`)**: [CC BY 4.0](./assets/LICENSE-ART.md) — Creative Commons Attribution 4.0 International.
