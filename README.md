# FatDuck

🌐 **Live Web Version (Desktop & Mobile Touch): [https://stbange.github.io/cooking/](https://stbange.github.io/cooking/)**  
📖 **Online Help & Culinary Science Manual: [docs/HELP.md](docs/HELP.md)**

**FatDuck** is a cross-platform first-principles culinary simulator and recipe invention engine written in **C11**, **SDL3**, **OpenGL (GL 3.3 Core / WebGL2)**, **Nuklear**, and **Lua 5.4**.

Instead of treating recipes as static text, FatDuck simulates cooking on a **3D volumetric voxel grid** driven by mathematical, physical, chemical, biological, and nutritional laws—reproducing classic dishes and inventing brand-new, physically working recipes from your constraints.

---

## Key Capabilities

- **Holistic Multi-Vessel & Movement 3D Engine (`src/core/`)**:
  - **Multi-Station Kitchen (`fd_kitchen_t`)**: Simulates up to 6 simultaneous stations (`Prep Board`, `Mixing/Whisk Bowl`, `Skillet`, `Stockpot`, `Oven/Bath`, `Resting Plate`) with staged additions, parallel cooking, fond deglazing, and enthalpy-conserving cross-vessel transfers ($T_{\text{mix}}$).
  - **Movement & Prep Mechanics**: Knife dicing/slicing/mincing (`mm` 3D voxel resizing, cellular aroma/amine rupture, and un-rested carving purge loss), pounding tenderization, high-shear whisking/whipping (droplet breakup & air foam porosity), dough kneading (viscoelastic `gluten_network` gas retention), gentle folding vs harsh stirring, and off-heat resting juice reabsorption.
  - **Physics, Chemistry, Biology & Nutrition**: Choi–Okos properties ($k(T), c_p(T), \rho(T)$), 3-pass Thomas ADI 3D heat & moisture diffusion, Arrhenius Maillard/caramelization/pyrolysis/6-stage protein/collagen/starch kinetics, Krieger–Dougherty emulsion rheology & irreversible egg curdling, 3D cold-spot Bigelow $D/z$ microbial lethality, and cooked glycemic nutrition ($\text{eGI}/\text{eGL}$).
- **Modern Nuklear UI, Chef Recipe Card, Guided Tutorial & Online Help**:
  - Clean dark-slate & saffron-gold **Nuklear** interface with **Google Noto Sans (SIL OFL 1.1)** typography, multi-vessel switcher, live movement action bar, step-by-step recipe executor, full **Chef Recipe Card** (exact gram masses, `mm` cuts, vessels, temperatures, durations, and scientific cues), `XY`/`XZ`/`YZ` 3D cross-section heatmaps, **6-step Interactive Guided Tour**, and **5-tab Help Modal (`F1`)** linked to [docs/HELP.md](docs/HELP.md).
  - Automatically switches to thumb-friendly touch controls (`>=48px` targets + bottom navigation) on mobile browsers and narrow viewports.
- **20-Language Internationalization (`assets/locales/*.lua`)**:
  - Instant runtime switching across the **Top 20 spoken world languages** (`en`, `zh`, `hi`, `es`, `fr`, `ar`, `bn`, `pt`, `ru`, `ur`, `id`, `de`, `ja`, `sw`, `mr`, `te`, `tr`, `ta`, `vi`, `ko`), with Arabic/Urdu contextual RTL shaping, dynamic active-script font atlas baking, Web browser locale auto-detection, and `localStorage` persistence.
- **Executable Classic Proposal & Multi-Station De Novo Invention (`fd_solver`)**:
  - Matches and chemically substitutes across **34+ multi-vessel classic recipes**, or runs a deterministic frame-sliced $(\mu + \lambda)$ multi-fidelity 3D evolutionary search (`8×8×8` screening $\to$ full 3D verification) across **107+ USDA-grounded ingredients** and **20 thermal & movement techniques** to synthesize complete executable workflows offline.
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
