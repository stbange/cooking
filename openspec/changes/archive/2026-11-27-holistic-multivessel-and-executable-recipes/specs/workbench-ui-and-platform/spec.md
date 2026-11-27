## ADDED Requirements

### Requirement: Multi-vessel workbench controls and Chef Recipe Card
The Kitchen Lab UI SHALL let the user inspect and switch among active kitchen vessels/stations, trigger live mechanical/non-thermal actions (Cut/Dice, Whisk/Whip, Knead/Fold, Flip, Deglaze/Rest, Combine), step through recipes one stage at a time or simulate all stages, and open a detailed executable Chef Recipe Card.

#### Scenario: Switching vessels and stepping through an executable recipe in Kitchen Lab
- **WHEN** a multi-vessel recipe is loaded in Kitchen Lab and the user advances steps or selects different vessels
- **THEN** the 3D slice inspector and live telemetry reflect the selected vessel and the Chef Recipe Card displays detailed step-by-step instructions
