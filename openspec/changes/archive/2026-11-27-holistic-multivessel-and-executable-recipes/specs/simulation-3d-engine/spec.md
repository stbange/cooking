## ADDED Requirements

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
