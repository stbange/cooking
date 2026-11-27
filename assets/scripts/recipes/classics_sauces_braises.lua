-- 11 Classic Emulsions, Mother Sauces & Collagen Braises
local recipes = {
  {
    id = "classic_hollandaise", name = "Classic Hollandaise Emulsion", category = "emulsion",
    notes = "Warm yolk lecithin oil-in-water emulsion held at 63°C below 73°C yolk curdling limit.",
    shape = "liquid", lx_mm = 120, ly_mm = 120, lz_mm = 15,
    items = {
      { id = "butter", mass_g = 120 }, { id = "egg_yolk", mass_g = 54 },
      { id = "lemon_juice", mass_g = 16 }, { id = "water", mass_g = 20 }, { id = "sea_salt", mass_g = 1.5 }
    },
    steps = { { technique = "whisk_emulsify", temp_c = 67, duration_sec = 360, flip_midway = false } }
  },
  {
    id = "classic_mayonnaise", name = "Dijon Mayonnaise", category = "emulsion",
    notes = "High-internal-phase cold oil-in-water emulsion stabilized by yolk lecithin and acidified to pH<=4.1.",
    shape = "liquid", lx_mm = 80, ly_mm = 80, lz_mm = 40,
    items = {
      { id = "canola_oil", mass_g = 150 }, { id = "egg_yolk", mass_g = 22 },
      { id = "dijon_mustard", mass_g = 12 }, { id = "lemon_juice", mass_g = 18 },
      { id = "white_wine_vin", mass_g = 8 }, { id = "sea_salt", mass_g = 1.8 }
    },
    steps = { { technique = "whisk_emulsify", temp_c = 22, duration_sec = 420, flip_midway = false } }
  },
  {
    id = "provencal_aioli", name = "Garlic Olive Oil Aioli", category = "emulsion",
    notes = "Fructan & saponin emulsifiers from crushed garlic partner with acidified egg yolk to suspend olive oil.",
    shape = "liquid", lx_mm = 80, ly_mm = 80, lz_mm = 40,
    items = {
      { id = "olive_oil", mass_g = 130 }, { id = "garlic", mass_g = 18 },
      { id = "egg_yolk", mass_g = 20 },   { id = "lemon_juice", mass_g = 22 },
      { id = "white_wine_vin", mass_g = 8 }, { id = "sea_salt", mass_g = 1.6 }
    },
    steps = { { technique = "whisk_emulsify", temp_c = 22, duration_sec = 420, flip_midway = false } }
  },
  {
    id = "beurre_blanc", name = "Shallot White-Wine Beurre Blanc", category = "emulsion",
    notes = "Acidic white wine reduction emulsified with cold butter dairy phospholipids at 58°C.",
    shape = "liquid", lx_mm = 80, ly_mm = 80, lz_mm = 35,
    items = {
      { id = "butter", mass_g = 115 }, { id = "white_wine", mass_g = 50 },
      { id = "white_wine_vin", mass_g = 18 }, { id = "shallot", mass_g = 25 },
      { id = "heavy_cream", mass_g = 20 }, { id = "sea_salt", mass_g = 1.4 }
    },
    steps = { { technique = "whisk_emulsify", temp_c = 58, duration_sec = 210, flip_midway = false } }
  },
  {
    id = "veloute_roux_sauce", name = "Golden Chicken Veloute", category = "sauce",
    notes = "Toasted flour roux starch gelatinizes in chicken stock to form a heat-stable viscous sauce.",
    shape = "liquid", lx_mm = 100, ly_mm = 100, lz_mm = 60,
    items = {
      { id = "chicken_stock", mass_g = 260 }, { id = "butter", mass_g = 25 },
      { id = "ap_flour", mass_g = 25 },       { id = "sea_salt", mass_g = 1.8 }
    },
    steps = {
      { technique = "saute_stirfry", temp_c = 135, duration_sec = 120, flip_midway = false },
      { technique = "simmer_poach",  temp_c = 90,  duration_sec = 480, flip_midway = false }
    }
  },
  {
    id = "cacio_e_pepe", name = "Roman Cacio e Pepe", category = "emulsion",
    notes = "Gelatinized pasta starch shields Pecorino casein from stringy thermal flocculation.",
    shape = "cylinder", lx_mm = 90, ly_mm = 12, lz_mm = 12,
    items = {
      { id = "pasta_semolina", mass_g = 110 }, { id = "water", mass_g = 190 },
      { id = "pecorino", mass_g = 55 },        { id = "black_pepper", mass_g = 3 }
    },
    steps = {
      { technique = "rolling_boil",   temp_c = 100, duration_sec = 510, flip_midway = false },
      { technique = "whisk_emulsify", temp_c = 64,  duration_sec = 90,  flip_midway = false }
    }
  },
  {
    id = "pasta_carbonara", name = "Traditional Roman Carbonara", category = "emulsion",
    notes = "Rendered pork fat, Pecorino, and egg yolk emulsified with starchy pasta water at 65°C.",
    shape = "cylinder", lx_mm = 90, ly_mm = 12, lz_mm = 12,
    items = {
      { id = "pasta_semolina", mass_g = 110 }, { id = "pork_belly", mass_g = 55 },
      { id = "egg_yolk", mass_g = 40 },        { id = "pecorino", mass_g = 35 },
      { id = "water", mass_g = 180 },          { id = "black_pepper", mass_g = 2.5 }
    },
    steps = {
      { technique = "rolling_boil",   temp_c = 100, duration_sec = 520, flip_midway = false },
      { technique = "whisk_emulsify", temp_c = 65,  duration_sec = 95,  flip_midway = false }
    }
  },
  {
    id = "boeuf_bourguignon", name = "Boeuf Bourguignon Collagen Braise", category = "braise",
    notes = "2-hour 88°C red-wine braise hydrolyzes tough chuck collagen into rich mouth-coating gelatin.",
    shape = "cube", lx_mm = 42, ly_mm = 42, lz_mm = 42,
    items = {
      { id = "beef_chuck", mass_g = 280 }, { id = "red_wine", mass_g = 120 },
      { id = "beef_stock", mass_g = 120 }, { id = "carrot", mass_g = 50 },
      { id = "yellow_onion", mass_g = 50 },{ id = "cremini_mushroom", mass_g = 50 },
      { id = "tomato_paste", mass_g = 15 },{ id = "sea_salt", mass_g = 3.2 }
    },
    steps = {
      { technique = "pan_sear", temp_c = 205, duration_sec = 180,  flip_midway = true },
      { technique = "braise",   temp_c = 89,  duration_sec = 7200, flip_midway = false, lid_closed = true }
    }
  },
  {
    id = "braised_pork_belly", name = "Soy-Caramel Braised Pork Belly", category = "braise",
    notes = "Slow collagen hydrolysis paired with Maillard/caramel glaze from soy, ginger, and muscovado.",
    shape = "fat_cap", lx_mm = 80, ly_mm = 65, lz_mm = 36,
    items = {
      { id = "pork_belly", mass_g = 260 }, { id = "soy_sauce", mass_g = 32 },
      { id = "brown_sugar", mass_g = 18 }, { id = "chicken_stock", mass_g = 140 },
      { id = "fresh_ginger", mass_g = 10 }
    },
    steps = {
      { technique = "pan_sear", temp_c = 190, duration_sec = 180,  flip_midway = true },
      { technique = "braise",   temp_c = 90,  duration_sec = 6600, flip_midway = false, lid_closed = true }
    }
  },
  {
    id = "french_onion_soup", name = "Caramelized French Onion Gratin", category = "braise",
    notes = "40-minute Maillard + fructose caramelization of onions simmered in beef bone stock with Gruyere.",
    shape = "slab", lx_mm = 90, ly_mm = 90, lz_mm = 30,
    items = {
      { id = "yellow_onion", mass_g = 300 }, { id = "butter", mass_g = 28 },
      { id = "beef_stock", mass_g = 220 },   { id = "white_wine", mass_g = 40 },
      { id = "gruyere", mass_g = 45 },       { id = "sea_salt", mass_g = 2.4 }
    },
    steps = {
      { technique = "saute_stirfry", temp_c = 155, duration_sec = 1800, flip_midway = true },
      { technique = "simmer_poach",  temp_c = 90,  duration_sec = 1200, flip_midway = false }
    }
  },
  {
    id = "umami_bone_jus", name = "Roasted Beef & Shiitake Umami Jus", category = "sauce",
    notes = "Synergistic glutamate + 5'-ribonucleotide amplification (1 + gamma*[IMP+GMP]) via reduction.",
    shape = "liquid", lx_mm = 100, ly_mm = 100, lz_mm = 50,
    items = {
      { id = "beef_stock", mass_g = 300 }, { id = "shiitake", mass_g = 45 },
      { id = "tomato_paste", mass_g = 18 },{ id = "shallot", mass_g = 25 },
      { id = "butter", mass_g = 20 },      { id = "soy_sauce", mass_g = 10 }
    },
    steps = { { technique = "simmer_poach", temp_c = 94, duration_sec = 1800, flip_midway = false } }
  }
}

for _, r in ipairs(recipes) do
  fatduck.add_recipe(r)
end
