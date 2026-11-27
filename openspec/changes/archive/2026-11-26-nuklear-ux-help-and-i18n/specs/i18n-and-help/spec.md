## ADDED Requirements

### Requirement: Twenty-language external Lua internationalization
The engine SHALL ship and load external Lua locale files in `./assets/locales/<code>.lua` covering the top 20 spoken world languages (`en`, `zh`, `hi`, `es`, `fr`, `ar`, `bn`, `pt`, `ru`, `ur`, `id`, `de`, `ja`, `sw`, `mr`, `te`, `tr`, `ta`, `vi`, `ko`), falling back to `en` for missing keys and dynamically baking the active locale's Unicode glyphs.

#### Scenario: Switching languages at runtime
- **WHEN** the user selects any of the 20 languages via the language switcher or `fd_app_set_language`
- **THEN** UI labels, tooltips, tutorial steps, and help pages update immediately and the font atlas rebuilds cleanly for the target script

### Requirement: Arabic and Urdu contextual RTL shaping
The i18n text shaper SHALL convert Arabic and Urdu strings to connected Arabic Presentation Forms and visually reorder RTL runs while keeping embedded LTR numeric and unit tokens readable.

#### Scenario: Shaping Arabic text with embedded temperature unit
- **WHEN** `fd_i18n_shape_text` formats an Arabic string containing `"180°C"`
- **THEN** Arabic letters convert to contextual presentation forms in visual RTL order while `"180°C"` remains left-to-right

### Requirement: Interactive guided tutorial and tabbed online help
The workbench SHALL provide a 6-step interactive guided tutorial with live demonstration actions and a 5-tab contextual help modal (`F1` / Help button) linking to online documentation (`docs/HELP.md`).

#### Scenario: Walking through the guided tutorial and opening help
- **WHEN** the user starts the tutorial and steps through all 6 stages or toggles the help modal
- **THEN** each tutorial step updates its live demonstration state and all 5 help sections are accessible and localized
