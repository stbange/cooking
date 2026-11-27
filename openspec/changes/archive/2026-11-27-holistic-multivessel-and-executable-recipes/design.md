# Design: Holistic Multi-Vessel Kitchen & Movement Mechanics

## Context
`fd_sim_state_t` already models a single 3D volumetric food/liquid state with Choi–Okos properties, 3-pass Thomas ADI heat/moisture PDEs, Arrhenius kinetics, Krieger–Dougherty rheology, Bigelow cold-spot lethality, and glycemic nutrition. Following Ponytail (`AGENTS.md`), we avoid duplicating any PDE solver by making `fd_kitchen_t` a lightweight coordinator over `fd_sim_state_t vessels[FD_MAX_VESSELS]`.

## Decisions
1. **Re-use `fd_sim_state_t` per Vessel**:
   - Each station (`Prep Board`, `Mixing Bowl`, `Skillet`, `Stockpot`, `Oven`, `Resting Plate`) is a standard `fd_sim_state_t` augmented with compact mechanical state scalars (`cell_rupture`, `foam_air`, `emulsion_shear_integral`, `gluten_network`, `mech_tender`, `rested_frac`, `fond_maillard`, `burner_on`, `consumed`).
   - `fd_sim_run_recipe` delegates directly to `fd_kitchen_run_recipe` and copies the final plated vessel into `*sim`, keeping 100% compatibility with all existing callers while upgrading them to full multi-vessel execution.
2. **Enthalpy & Mass Conservation on Staged Additions and Transfers**:
   - Adding ingredients at step `s` (`add_at_step`) or combining `src -> dst` (`FD_ACT_COMBINE`, `FD_ACT_FOLD_STIR`, `FD_ACT_DEGLAZE`) conserves mass pools and thermal enthalpy $T_{\text{mix}} = (C_{\text{dst}} \bar{T}_{\text{dst}} + C_{\text{src}} \bar{T}_{\text{src}}) / (C_{\text{dst}} + C_{\text{src}})$.
   - Combining a liquid sauce/emulsion with a 3D solid cut preserves the 3D voxel geometry of the solid cut while equilibrating boundary temperatures, transferring dissolved Maillard fond/aroma, checking thermal curdling (`>72.5°C` unshielded yolk), and tracking worst-case microbial lethality (`min` cold-spot log10 reduction across unpasteurized components).
3. **Deterministic Chef Instruction Synthesis (`fd_recipe_describe_step`)**:
   - Authored Lua steps can provide explicit `instruction` and `why` strings; when omitted (including in adapted or evolutionary invented recipes), C generates exact gram/mm/vessel/temperature/time instructions and physical doneness cues automatically.
