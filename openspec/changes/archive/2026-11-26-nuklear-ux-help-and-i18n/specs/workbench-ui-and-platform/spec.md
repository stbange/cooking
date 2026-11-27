## ADDED Requirements

### Requirement: Nuklear modern UI rendering with external TrueType/OpenType fonts
The application SHALL render its immediate-mode UI using Nuklear (`NK_INCLUDE_VERTEX_BUFFER_OUTPUT`) styled with a modern dark-slate/saffron palette, rounded controls, scissor clipping, smooth color-legend cross-sections, and external unbaked Noto fonts loaded from `./assets/fonts/`.

#### Scenario: Rendering workbench with Nuklear indexed draw lists
- **WHEN** `fd_app_update_and_render` runs a frame on Desktop or Mobile Touch
- **THEN** Nuklear generates non-empty indexed vertex/element buffers and draw commands using the dynamically baked font atlas texture
