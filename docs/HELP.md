# FatDuck Online Help & Culinary Science Manual

Welcome to **FatDuck** — a cross-platform, first-principles 3D cooking simulator and recipe invention laboratory.

🌐 **Play / Run Online**: [https://stbange.github.io/cooking/](https://stbange.github.io/cooking/)

---

## 1. Quick Start

1. **Kitchen Lab**:
   - Pick any dish from the **Dish & Recipe** dropdown (33+ recipes) or browse with `< Prev` / `Next >`.
   - Switch **Cooking Technique** (Pan Sear, Deep Fry, Convection Bake, Sous-Vide, Boil, Braise, Steam, Emulsify, Ferment, etc.) and adjust **Vessel Temperature (°C)** and **Simulation Speed (`1x`–`120x`)**.
   - Watch the live **3D Cross-Section Inspector** (`XY Top`, `XZ Side`, `YZ End`), toggle overlays (**Temp**, **Moisture**, **Browning**, **Protein**, **Safety**), and click **Cold Spot** or **Peak Crust** to jump straight to the coldest interior voxel or hottest crust voxel.
   - Click **Simulate Full Recipe** to run every programmed recipe step to completion.
2. **Classics (Propose)**:
   - Toggle dietary filters (**Vegan**, **Gluten-Free**, **Dairy-Free**, **Low-GI `<= 52`**) and **Max Cook Time**, then click **Find Verified Classics**.
   - Incompatible ingredients are replaced automatically using functional chemistry similarity, and every adapted recipe is verified in 3D.
3. **Invent**:
   - Set desired **Target Crispness**, **Target Umami**, **Min Protein (g)**, dietary filters, and **Emulsion** requirement, then click **Invent Recipe**.
   - FatDuck's offline multi-fidelity evolutionary solver screens candidates on an `8×8×8` coarse grid and verifies top dishes in full 3D.
4. **Interactive Guided Tour & In-App Help**:
   - Click **Tutorial** in the top bar anytime for the 6-step interactive hands-on walkthrough.
   - Click **Help** (or press `F1`) for the 5-tab in-app reference guide.

---

## 2. First-Principles 3D Culinary Science

| Domain | Models Implemented |
| :--- | :--- |
| **3D Geometry** | 9 volumetric cut models (`slab`, `cube`, `cylinder`, `sphere`, `tapered`, `fat_cap`, `bone_in`, `egg`, `liquid`) with 6-face surface masks and 3D flip/toss. |
| **Physics** | Choi–Okos (1986) temperature-dependent density, specific heat & conductivity; unconditionally stable 3-pass Thomas ADI 3D heat & moisture diffusion; latent ice fusion & boiling desiccation plateau; Lewis psychrometric evaporation; oil uptake/fat rendering; Krieger–Dougherty emulsion rheology. |
| **Chemistry** | Arrhenius kinetics for Maillard browning (`> 140°C`), caramelization (`> 160°C`), pyrolysis/char, 6-stage protein denaturation & actin syneresis, collagen-to-gelatin hydrolysis (`> 68°C`), starch gelatinization & `4°C` RS3 retrogradation, logarithmic Henderson–Hasselbalch aqueous `pH`, and oil smoke point. |
| **Biology & Sensory** | 3D cold-spot Bigelow $D/z$ microbial lethality (`SAFE >= 6.5D` or acid safety `pH <= 4.25`), yeast & lactic fermentation, amylase activity, 5-taste + umami synergy, volatile aroma, and 3D crust/core texture scoring. |
| **Nutrition & Digestion** | Cooked mass-balance energy & macros (`kcal`, protein, fat, net carbs, RS3 resistant starch), Estimated Glycemic Index/Load (`eGI`/`eGL`), and protein bioavailability. |

> **Educational Notice**: Thermal lethality and nutrition figures are first-principles educational simulation outputs, not clinical or regulatory food-safety certifications.

---

## 3. Controls & Shortcuts

### Desktop
- `Space` — Pause / Resume live simulation
- `F` — Flip / Toss 3D food stack
- `X` or `Tab` — Cycle `XY` / `XZ` / `YZ` cross-section plane
- `1`–`5` — Set simulation speed (`1x`, `5x`, `15x`, `45x`, `120x`)
- `F1` — Open / Close Help modal
- `F5` — Hot-reload `./assets/scripts/` and `./assets/locales/`
- `Esc` — Close open Help/Tutorial overlay (or quit when no modal is open)

### Mobile & Touch
- Automatically activates thumb-friendly `>= 48px` controls and bottom tab navigation on phones/tablets or viewports `< 820px` wide (you can also switch manually via the top-right `Desktop` / `Touch` button).

---

## 4. Languages (Top 20 Spoken World Languages)

Switch languages anytime from the top header dropdown (saved automatically in `localStorage` on Web, and auto-detected from browser locale on first visit):

English (`en`), 中文 简体 (`zh`), हिन्दी (`hi`), Español (`es`), Français (`fr`), العربية (`ar`, RTL), বাংলা (`bn`), Português (`pt`), Русский (`ru`), اردو (`ur`, RTL), Bahasa Indonesia (`id`), Deutsch (`de`), 日本語 (`ja`), Kiswahili (`sw`), मराठी (`mr`), తెలుగు (`te`), Türkçe (`tr`), தமிழ் (`ta`), Tiếng Việt (`vi`), 한국어 (`ko`).

---

## 5. Modding & Extending FatDuck

All assets stay **unbaked** inside `./assets/` beside the executable (and loose via `./assets/manifest.json` on GitHub Pages):
- **Ingredients**: `assets/scripts/ingredients/*.lua`
- **Techniques & Kinetics**: `assets/scripts/techniques.lua`, `assets/scripts/reactions.lua`
- **Recipes**: `assets/scripts/recipes/*.lua`
- **Custom Mods & Hooks**: `assets/scripts/mods/*.lua` (`on_init`, `on_step_post`, `on_score_dish`)
- **Translations**: `assets/locales/<code>.lua`
- **Fonts**: `assets/fonts/*.ttf` / `*.otf` (SIL OFL 1.1 Noto subsets)

Edit any Lua script or translation file and press `F5` (or click **Hot-Reload Scripts** in the **Mod Lab** tab) — no C compiler needed.
