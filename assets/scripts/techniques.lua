-- 20 First-Principles Thermal, Mechanical Movement & Prep Techniques
local techniques = {
  { id = "pan_sear",         name = "Pan Sear",              h_bottom = 480, h_top = 14,  h_side = 22,  emissivity_rad = 0.05, ambient_humidity = 0.20, default_temp_c = 205, min_temp_c = 130, max_temp_c = 250, oil_uptake_rate = 0.15 },
  { id = "saute_stirfry",    name = "Saute / Stir-Fry",      h_bottom = 420, h_top = 28,  h_side = 85,  emissivity_rad = 0.05, ambient_humidity = 0.25, default_temp_c = 195, min_temp_c = 120, max_temp_c = 245, oil_uptake_rate = 0.18, shear_rate = 18 },
  { id = "butter_baste",     name = "Butter Baste",          h_bottom = 410, h_top = 190, h_side = 160, emissivity_rad = 0.02, ambient_humidity = 0.30, default_temp_c = 165, min_temp_c = 120, max_temp_c = 190, oil_uptake_rate = 0.22 },
  { id = "simmer_poach",     name = "Simmer / Poach",        h_bottom = 520, h_top = 480, h_side = 500, emissivity_rad = 0.0,  ambient_humidity = 0.98, default_temp_c = 88,  min_temp_c = 62,  max_temp_c = 96,  immersed_water = true, shear_rate = 6 },
  { id = "rolling_boil",     name = "Rolling Boil",          h_bottom = 950, h_top = 850, h_side = 900, emissivity_rad = 0.0,  ambient_humidity = 1.00, default_temp_c = 100, min_temp_c = 97,  max_temp_c = 102, immersed_water = true, shear_rate = 25 },
  { id = "braise",           name = "Covered Braise",        h_bottom = 260, h_top = 140, h_side = 220, emissivity_rad = 0.25, ambient_humidity = 0.92, default_temp_c = 92,  min_temp_c = 80,  max_temp_c = 135, immersed_water = true },
  { id = "steam",            name = "Steam",                 h_bottom = 620, h_top = 620, h_side = 620, emissivity_rad = 0.0,  ambient_humidity = 1.00, default_temp_c = 100, min_temp_c = 85,  max_temp_c = 105, immersed_water = true },
  { id = "convection_roast", name = "Convection Roast",      h_bottom = 65,  h_top = 52,  h_side = 55,  emissivity_rad = 0.65, ambient_humidity = 0.18, default_temp_c = 195, min_temp_c = 110, max_temp_c = 245 },
  { id = "bake",             name = "Deck / Radiant Bake",   h_bottom = 95,  h_top = 34,  h_side = 36,  emissivity_rad = 0.72, ambient_humidity = 0.35, default_temp_c = 210, min_temp_c = 135, max_temp_c = 255 },
  { id = "broil_grill",      name = "Broil / High IR Grill", h_bottom = 180, h_top = 65,  h_side = 45,  emissivity_rad = 0.92, ambient_humidity = 0.12, default_temp_c = 255, min_temp_c = 200, max_temp_c = 290 },
  { id = "sous_vide",        name = "Sous-Vide Bath",        h_bottom = 680, h_top = 680, h_side = 680, emissivity_rad = 0.0,  ambient_humidity = 1.00, default_temp_c = 56,  min_temp_c = 42,  max_temp_c = 90,  sealed_pouch = true },
  { id = "deep_fry",         name = "Deep Fry",              h_bottom = 640, h_top = 620, h_side = 630, emissivity_rad = 0.0,  ambient_humidity = 0.05, default_temp_c = 185, min_temp_c = 130, max_temp_c = 205, immersed_oil = true, oil_uptake_rate = 0.45 },
  { id = "whisk_emulsify",   name = "Warm Whisk / Emulsify", h_bottom = 520, h_top = 35,  h_side = 180, emissivity_rad = 0.0,  ambient_humidity = 0.55, default_temp_c = 62,  min_temp_c = 18,  max_temp_c = 100, shear_rate = 85 },
  { id = "cure_marinade",    name = "Acid Cure / Brine",     h_bottom = 80,  h_top = 80,  h_side = 80,  emissivity_rad = 0.0,  ambient_humidity = 0.95, default_temp_c = 8,   min_temp_c = 2,   max_temp_c = 26,  sealed_pouch = true },
  { id = "ferment_proof",    name = "Ferment / Proof",       h_bottom = 22,  h_top = 18,  h_side = 18,  emissivity_rad = 0.0,  ambient_humidity = 0.82, default_temp_c = 28,  min_temp_c = 4,   max_temp_c = 42,  sealed_pouch = true },
  { id = "knife_prep",       name = "Knife Cut / Mise",      h_bottom = 12,  h_top = 10,  h_side = 10,  emissivity_rad = 0.0,  ambient_humidity = 0.55, default_temp_c = 20,  min_temp_c = 4,   max_temp_c = 30 },
  { id = "whisk_whip",       name = "Cold Whip / Aerate",    h_bottom = 25,  h_top = 20,  h_side = 25,  emissivity_rad = 0.0,  ambient_humidity = 0.55, default_temp_c = 20,  min_temp_c = 4,   max_temp_c = 45,  shear_rate = 140 },
  { id = "knead_fold",       name = "Knead / Gentle Fold",   h_bottom = 18,  h_top = 14,  h_side = 14,  emissivity_rad = 0.0,  ambient_humidity = 0.65, default_temp_c = 24,  min_temp_c = 15,  max_temp_c = 38,  shear_rate = 28 },
  { id = "deglaze_reduce",   name = "Deglaze & Reduce",      h_bottom = 540, h_top = 60,  h_side = 240, emissivity_rad = 0.02, ambient_humidity = 0.70, default_temp_c = 95,  min_temp_c = 75,  max_temp_c = 125, immersed_water = true, shear_rate = 22 },
  { id = "rest_hold",        name = "Off-Heat Carryover Rest", h_bottom = 10, h_top = 9,  h_side = 9,   emissivity_rad = 0.05, ambient_humidity = 0.55, default_temp_c = 26,  min_temp_c = 18,  max_temp_c = 55 }
}

for _, t in ipairs(techniques) do
  fatduck.add_technique(t)
end
