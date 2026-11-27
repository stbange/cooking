# simulation-3d-engine Specification

## Purpose

Simulates cooking from mathematical, physical, chemical, biological, and nutritional first principles on a 3D volumetric voxel grid.

## Requirements

### Requirement: 3D anisotropic heat, phase change, and moisture transport
The simulation SHALL discretize solid foods onto a 3D `Nx x Ny x Nz` voxel grid with Choi–Okos constituent properties, unconditionally stable 3-pass Thomas ADI diffusion, enthalpy ice fusion, boiling desiccation plateau, and Fickian moisture diffusion.

#### Scenario: 3D corner heating exceeds edge, face, and core
- **WHEN** an isotropic 3D food cube is roasted convectively
- **THEN** 3-face corner voxels heat faster than 2-face edge voxels, which heat faster than 1-face center voxels and the 3D interior core

### Requirement: Arrhenius reaction kinetics and emulsion stability
The simulation SHALL advance Maillard browning, caramelization, pyrolysis, multi-stage protein denaturation, collagen-to-gelatin hydrolysis, starch gelatinization/retrogradation, pH buffering, and Krieger–Dougherty emulsion stability.

#### Scenario: Thermal curdling of overheated yolk emulsion
- **WHEN** an unshielded yolk-butter emulsion is heated above 85°C
- **THEN** `emulsion_stability` drops below `0.6` indicating split/curdling

### Requirement: Cold-spot microbial lethality and glycemic bioavailability
The simulation SHALL integrate Bigelow $D/z$ lethality at the coldest 3D tissue voxel and compute cooked macronutrients, resistant starch RS3 retrogradation, Estimated Glycemic Index (`eGI`), and protein bioavailability.

#### Scenario: Chilled retrogradation lowers potato eGI
- **WHEN** boiled potatoes undergo a 4°C chill hold (`chill_after = true`)
- **THEN** retrograded resistant starch increases and `estimated_gi` decreases relative to hot freshly boiled potatoes

### Requirement: Multi-vessel kitchen coordination and enthalpy-conserving transfers
The simulation engine SHALL manage multiple simultaneous stations (`fd_kitchen_t`, up to `FD_MAX_VESSELS` vessels), stage ingredient additions per step, and conserve constituent mass, thermal enthalpy, and worst-case microbial safety when deglazing or combining vessels.

#### Scenario: Tempering off-heat Carbonara emulsion vs overheating
- **WHEN** boiled pasta and rendered pork skillet are combined and tempered off-heat (`~64°C`) with whisked raw egg yolk and Pecorino from a mixing bowl
- **THEN** the equilibrium mix temperature pasteurizes the dish safely while maintaining high `emulsion_stability`, whereas combining into an overheated skillet (`>85°C` mix) curdles the yolk emulsion

### Requirement: Non-thermal mechanical and movement transformations
The simulation engine SHALL model physical and chemical state changes from mechanical prep and movement—including knife cutting/dicing (3D voxel resizing and cellular rupture), pounding tenderization, whisking/whipping (shear emulsification and foam porosity), kneading (gluten gas retention), gentle folding, fond deglazing, and resting juice reabsorption.

#### Scenario: Gentle folding preserves whipped batter foam compared to harsh stirring
- **WHEN** a bowl of whipped egg-sugar foam is combined into melted chocolate/flour via gentle folding (`agitation <= 0.35`) versus harsh high-shear stirring
- **THEN** the gently folded batter retains significantly higher `avg_porosity` and baked `tenderness` than the harshly stirred batter
