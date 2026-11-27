## ADDED Requirements

### Requirement: Multi-vessel and movement-action recipe schema
The Lua `fatduck.add_recipe` registry SHALL accept optional `vessels` station definitions, per-item `vessel`, `add_step`, and `prep` metadata, and step-level `action`, `vessel`, `target_vessel`, `agitation`, `parallel`, `cut_lx_mm`/`cut_ly_mm`/`cut_lz_mm`, `instruction`, and `why` fields while remaining backward-compatible with single-vessel Lua recipes.

#### Scenario: Loading multi-vessel and legacy single-vessel Lua recipes
- **WHEN** `fd_lua_reload_all` loads recipes with explicit `vessels` alongside compact single-vessel recipes
- **THEN** multi-vessel recipes populate their station definitions and step actions, and compact recipes default cleanly to a primary vessel
