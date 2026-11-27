# lua-data-and-modding Specification

## Purpose

Hosts a sandboxed Lua 5.4 runtime that loads declarative tables for ingredients, techniques, reaction kinetics, and classic recipes, and dispatches safe mod hooks without exposing host OS shell or filesystem primitives.

## Requirements

### Requirement: Sandboxed Lua execution
The Lua host SHALL strip `os`, `io`, `package`, and `debug` libraries and enforce an instruction-count hook limit so user or web-edited Lua scripts cannot hang the application or access unauthorized host resources.

#### Scenario: Infinite loop script termination
- **WHEN** a modded Lua script enters an infinite `while true do end` loop
- **THEN** `fd_lua` aborts execution cleanly once the instruction quota is exceeded and records a readable error string

### Requirement: Declarative culinary database loading
The Lua host SHALL load ingredients, cooking techniques, reaction parameters, and classic recipes from external `.lua` files in `assets/scripts/` into validated C structs.

#### Scenario: Loading ingredients and mods on hot-reload
- **WHEN** `fd_lua_reload_all` executes `scripts/init.lua` and `scripts/mods/*.lua`
- **THEN** all valid ingredient, technique, and recipe entries populate the C registry and any `on_init` mod hook runs

### Requirement: Multi-vessel and movement-action recipe schema
The Lua `fatduck.add_recipe` registry SHALL accept optional `vessels` station definitions, per-item `vessel`, `add_step`, and `prep` metadata, and step-level `action`, `vessel`, `target_vessel`, `agitation`, `parallel`, `cut_lx_mm`/`cut_ly_mm`/`cut_lz_mm`, `instruction`, and `why` fields while remaining backward-compatible with single-vessel Lua recipes.

#### Scenario: Loading multi-vessel and legacy single-vessel Lua recipes
- **WHEN** `fd_lua_reload_all` loads recipes with explicit `vessels` alongside compact single-vessel recipes
- **THEN** multi-vessel recipes populate their station definitions and step actions, and compact recipes default cleanly to a primary vessel
