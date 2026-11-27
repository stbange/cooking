-- 10 Classic Benchmark Protein, Seafood, Egg & Tofu Recipes
local recipes = {
  {
    id = "reverse_sear_ribeye", name = "Reverse-Sear Bone-In Ribeye", category = "protein",
    notes = "Low convective roast to 52°C core pasteurization followed by blazing cast-iron Maillard crust.",
    shape = "bone_in", lx_mm = 110, ly_mm = 85, lz_mm = 32,
    items = {
      { id = "beef_ribeye", mass_g = 320 }, { id = "clarified_butter", mass_g = 18 },
      { id = "sea_salt", mass_g = 3.5 },    { id = "rosemary", mass_g = 4 }, { id = "garlic", mass_g = 6 }
    },
    steps = {
      { technique = "convection_roast", temp_c = 135, duration_sec = 2100, flip_midway = false },
      { technique = "pan_sear",         temp_c = 225, duration_sec = 120,  flip_midway = true }
    }
  },
  {
    id = "butter_basted_duck", name = "Crispy-Skin Butter-Basted Duck Breast", category = "protein",
    notes = "Renders subcutaneous fat cap while keeping lean breast rose and juicy via arroser basting.",
    shape = "fat_cap", lx_mm = 120, ly_mm = 75, lz_mm = 26,
    items = {
      { id = "duck_breast", mass_g = 240 }, { id = "butter", mass_g = 22 },
      { id = "sea_salt", mass_g = 2.6 },    { id = "fresh_thyme", mass_g = 3 }
    },
    steps = {
      { technique = "pan_sear",         temp_c = 178, duration_sec = 360, flip_midway = false },
      { technique = "butter_baste",     temp_c = 165, duration_sec = 180, flip_midway = true },
      { technique = "convection_roast", temp_c = 180, duration_sec = 360, flip_midway = false }
    }
  },
  {
    id = "onsen_egg_63c", name = "63°C Heston Onsen Egg", category = "egg",
    notes = "Isothermal sous-vide equilibrium where yolk thickens into custard while ovotransferrin barely sets.",
    shape = "egg", lx_mm = 56, ly_mm = 44, lz_mm = 44,
    items = { { id = "egg_whole", mass_g = 60 }, { id = "dashi_stock", mass_g = 20 }, { id = "soy_sauce", mass_g = 4 } },
    steps = { { technique = "sous_vide", temp_c = 63.5, duration_sec = 2700, flip_midway = false } }
  },
  {
    id = "soft_boiled_egg_6m", name = "7-Minute Jammy Pasteurized Egg", category = "egg",
    notes = "Steep radial 3D thermal gradient sets outer albumen at >80°C while pasteurizing jammy yolk core ~63°C.",
    shape = "egg", lx_mm = 52, ly_mm = 40, lz_mm = 40,
    items = { { id = "egg_whole", mass_g = 58 }, { id = "sea_salt", mass_g = 0.6 } },
    steps = { { technique = "rolling_boil", temp_c = 100, duration_sec = 460, flip_midway = false } }
  },
  {
    id = "french_omelette", name = "Classic Silky French Omelette", category = "egg",
    notes = "High shear agitation in butter coagulates thin curds gently at 66°C without Maillard browning.",
    shape = "slab", lx_mm = 160, ly_mm = 110, lz_mm = 8,
    items = { { id = "egg_whole", mass_g = 150 }, { id = "butter", mass_g = 20 }, { id = "sea_salt", mass_g = 1.6 } },
    steps = { { technique = "saute_stirfry", temp_c = 160, duration_sec = 290, flip_midway = true, lid_closed = true } }
  },
  {
    id = "crispy_skin_salmon", name = "Crispy-Skin Tapered Salmon Fillet", category = "seafood",
    notes = "Skin-side sear desiccates collagen skin into glass-crispness, finished with brief kiss on flesh side.",
    shape = "tapered", lx_mm = 120, ly_mm = 65, lz_mm = 22,
    items = {
      { id = "salmon_fillet", mass_g = 190 }, { id = "canola_oil", mass_g = 10 },
      { id = "sea_salt", mass_g = 2.0 },      { id = "lemon_juice", mass_g = 6 }
    },
    steps = {
      { technique = "pan_sear",     temp_c = 198, duration_sec = 340, flip_midway = false },
      { technique = "butter_baste", temp_c = 168, duration_sec = 190, flip_midway = true }
    }
  },
  {
    id = "garlic_butter_shrimp", name = "Garlic-Butter Seared Shrimp", category = "seafood",
    notes = "Fast high-heat sear hits 6.5D pasteurization before actin over-contraction turns shrimp rubbery.",
    shape = "cylinder", lx_mm = 48, ly_mm = 20, lz_mm = 16,
    items = {
      { id = "shrimp", mass_g = 180 }, { id = "butter", mass_g = 24 },
      { id = "garlic", mass_g = 10 },  { id = "lemon_juice", mass_g = 8 }, { id = "sea_salt", mass_g = 1.8 }
    },
    steps = { { technique = "saute_stirfry", temp_c = 192, duration_sec = 220, flip_midway = true } }
  },
  {
    id = "sous_vide_chicken", name = "62°C Pasteurized Sous-Vide Chicken Breast", category = "protein",
    notes = "Time-at-temperature Bigelow pasteurization yields ultra-juicy chicken below actin squeeze threshold.",
    shape = "tapered", lx_mm = 125, ly_mm = 75, lz_mm = 28,
    items = {
      { id = "chicken_breast", mass_g = 210 }, { id = "olive_oil", mass_g = 10 },
      { id = "sea_salt", mass_g = 2.4 },       { id = "fresh_thyme", mass_g = 3 }
    },
    steps = {
      { technique = "sous_vide", temp_c = 62.5, duration_sec = 3000, flip_midway = false },
      { technique = "pan_sear",  temp_c = 210,  duration_sec = 60,   flip_midway = true }
    }
  },
  {
    id = "citrus_ceviche", name = "Lime-Cured Sea Cod Ceviche", category = "seafood",
    notes = "Low-pH citric acid denatures surface myosin cold without thermal heat.",
    shape = "cube", lx_mm = 18, ly_mm = 18, lz_mm = 18,
    items = {
      { id = "cod_fillet", mass_g = 180 }, { id = "lime_juice", mass_g = 55 },
      { id = "shallot", mass_g = 20 },     { id = "jalapeno", mass_g = 8 }, { id = "sea_salt", mass_g = 2.2 }
    },
    steps = { { technique = "cure_marinade", temp_c = 8, duration_sec = 1200, flip_midway = false } }
  },
  {
    id = "crispy_fried_tofu", name = "3D Corner-Crisp Fried Tofu Cubes", category = "plant_protein",
    notes = "Starch-dusted tofu cubes deep-fried for 3-axis golden corner crispness and pillowy core.",
    shape = "cube", lx_mm = 26, ly_mm = 26, lz_mm = 26,
    items = {
      { id = "firm_tofu", mass_g = 200 }, { id = "cornstarch", mass_g = 18 },
      { id = "canola_oil", mass_g = 25 }, { id = "soy_sauce", mass_g = 14 }, { id = "fresh_ginger", mass_g = 6 }
    },
    steps = { { technique = "deep_fry", temp_c = 188, duration_sec = 240, flip_midway = true } }
  }
}

for _, r in ipairs(recipes) do
  fatduck.add_recipe(r)
end
