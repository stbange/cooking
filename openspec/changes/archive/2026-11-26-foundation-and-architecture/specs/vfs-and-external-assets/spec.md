# Spec Delta

## Purpose

Provides unbaked external file access from `./assets/` next to the executable on Desktop and via `./assets/manifest.json` HTTP fetching plus live override buffers on Web.

## ADDED Requirements

### Requirement: Unbaked sibling asset loading
The engine SHALL load graphics, fonts, sounds, shaders, and Lua scripts from an external `assets/` directory adjacent to the executable (or working directory fallback) rather than embedding asset payloads in the compiled binary.

#### Scenario: Loading an external asset from disk
- **WHEN** `fd_vfs_read` requests `scripts/init.lua` on a native platform
- **THEN** the VFS resolves the path under `./assets/scripts/init.lua` and returns the raw byte buffer and length

### Requirement: Manifest parsing for Web HTTP asset fetching
The VFS SHALL parse `assets/manifest.json` into a bounded list of relative asset paths so the Emscripten build fetches loose files over HTTP without `--preload-file` bundling.

#### Scenario: Parsing valid manifest JSON array
- **WHEN** `fd_vfs_parse_manifest` is given a JSON string array of relative asset paths
- **THEN** it extracts each valid relative path while rejecting path-traversal sequences (`..`)

### Requirement: Live runtime and localStorage overrides
The VFS SHALL allow in-memory override buffers (backed by `localStorage` on Web or mod reloads on Desktop) to shadow files in `assets/scripts/`.

#### Scenario: Overriding a Lua script in memory
- **WHEN** `fd_vfs_set_override` registers modified content for `scripts/mods/example_mod.lua`
- **THEN** subsequent `fd_vfs_read` calls return the overridden content until cleared
