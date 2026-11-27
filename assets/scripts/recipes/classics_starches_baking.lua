-- 11 Classic Starches, Glycemic Benchmarks, Baking, Fermentation & Sugar Chemistry Recipes
local recipes = {
  {
    id = "pasta_pomodoro", name = "Pasta Al Dente al Pomodoro", category = "starch",
    notes = "Controlled starch gelatinization preserves firm core and moderate eGI with olive oil & tomato acid.",
    shape = "cylinder", lx_mm = 90, ly_mm = 10, lz_mm = 10,
    items = {
      { id = "pasta_semolina", mass_g = 110 }, { id = "water", mass_g = 200 },
      { id = "tomato", mass_g = 160 },         { id = "olive_oil", mass_g = 18 },
      { id = "garlic", mass_g = 8 },           { id = "sea_salt", mass_g = 2.2 }
    },
    steps = { { technique = "rolling_boil", temp_c = 100, duration_sec = 540, flip_midway = false } }
  },
  {
    id = "mushroom_risotto", name = "Creamy Shiitake Parmigiano Risotto", category = "starch",
    notes = "Shear agitation releases amylopectin from Arborio rice to self-emulsify butter and Parmesan.",
    shape = "liquid", lx_mm = 110, ly_mm = 110, lz_mm = 45,
    items = {
      { id = "arborio_rice", mass_g = 95 },  { id = "chicken_stock", mass_g = 280 },
      { id = "shiitake", mass_g = 60 },      { id = "butter", mass_g = 24 },
      { id = "parmesan", mass_g = 30 },      { id = "white_wine", mass_g = 35 }
    },
    steps = { { technique = "simmer_poach", temp_c = 92, duration_sec = 1140, flip_midway = false } }
  },
  {
    id = "triple_cooked_chips", name = "Fat Duck Triple-Cooked Chips", category = "starch",
    notes = "Heston Blumenthal benchmark: simmer to fissure starch, chill, low-fry blanch, and 190°C crisp!",
    shape = "cube", lx_mm = 65, ly_mm = 16, lz_mm = 16,
    items = {
      { id = "russet_potato", mass_g = 240 }, { id = "canola_oil", mass_g = 30 }, { id = "sea_salt", mass_g = 2.8 }
    },
    steps = {
      { technique = "simmer_poach", temp_c = 94,  duration_sec = 720, flip_midway = false },
      { technique = "deep_fry",     temp_c = 135, duration_sec = 300, flip_midway = false },
      { technique = "deep_fry",     temp_c = 190, duration_sec = 210, flip_midway = true }
    }
  },
  {
    id = "pommes_puree", name = "Robuchon Butter Pommes Puree", category = "starch",
    notes = "Precise simmer gelatinizes Yukon starch; butter fat coats granules for velvety richness.",
    shape = "cube", lx_mm = 32, ly_mm = 32, lz_mm = 32,
    items = {
      { id = "yukon_potato", mass_g = 250 }, { id = "butter", mass_g = 85 },
      { id = "whole_milk", mass_g = 60 },    { id = "sea_salt", mass_g = 2.8 }
    },
    steps = { { technique = "simmer_poach", temp_c = 92, duration_sec = 1350, flip_midway = false } }
  },
  {
    id = "chilled_potato_salad_rs3", name = "Chilled Vinaigrette Potato Salad (RS3 Low-GI)", category = "starch",
    notes = "4°C post-cook chill crystallizes retrograded RS3 resistant starch and vinegar slows gastric eGI.",
    shape = "cube", lx_mm = 26, ly_mm = 26, lz_mm = 26, chill_after = true,
    items = {
      { id = "yukon_potato", mass_g = 240 },   { id = "olive_oil", mass_g = 22 },
      { id = "white_wine_vin", mass_g = 18 },  { id = "dijon_mustard", mass_g = 10 },
      { id = "shallot", mass_g = 15 },         { id = "sea_salt", mass_g = 2.4 }
    },
    steps = { { technique = "simmer_poach", temp_c = 93, duration_sec = 1200, flip_midway = false } }
  },
  {
    id = "sourdough_boule", name = "Artisanal Sourdough Country Boule", category = "baking",
    notes = "Yeast + Lactobacillus proof builds CO2 porosity & organic acids; 230°C deck bake sets crisp crust.",
    shape = "sphere", lx_mm = 165, ly_mm = 165, lz_mm = 95,
    items = {
      { id = "bread_flour", mass_g = 300 }, { id = "water", mass_g = 215 },
      { id = "sourdough_starter", mass_g = 65 }, { id = "sea_salt", mass_g = 6.2 }
    },
    steps = {
      { technique = "ferment_proof", temp_c = 27,  duration_sec = 7200, flip_midway = false, lid_closed = true },
      { technique = "braise",        temp_c = 125, duration_sec = 1500, flip_midway = false, lid_closed = true },
      { technique = "bake",          temp_c = 195, duration_sec = 900,  flip_midway = false, lid_closed = false }
    }
  },
  {
    id = "brioche_roll", name = "Enriched Butter Brioche", category = "baking",
    notes = "Yeast fermentation in egg-and-butter enriched dough baked to tender golden crumb.",
    shape = "sphere", lx_mm = 95, ly_mm = 95, lz_mm = 70,
    items = {
      { id = "bread_flour", mass_g = 180 }, { id = "egg_whole", mass_g = 80 },
      { id = "butter", mass_g = 75 },       { id = "whole_milk", mass_g = 35 },
      { id = "sucrose", mass_g = 22 },      { id = "dry_yeast", mass_g = 4 }, { id = "sea_salt", mass_g = 3.2 }
    },
    steps = {
      { technique = "ferment_proof", temp_c = 29,  duration_sec = 5400, flip_midway = false, lid_closed = true },
      { technique = "bake",          temp_c = 185, duration_sec = 1320, flip_midway = false }
    }
  },
  {
    id = "salted_caramel_sauce", name = "Salted Amber Caramel Sauce", category = "confection",
    notes = "Sucrose pyrolysis above 170°C creates furanones and diacetyl, arrested with cream & butter.",
    shape = "liquid", lx_mm = 90, ly_mm = 90, lz_mm = 30,
    items = {
      { id = "sucrose", mass_g = 140 },     { id = "heavy_cream", mass_g = 100 },
      { id = "butter", mass_g = 40 },       { id = "sea_salt", mass_g = 2.5 }, { id = "vanilla_bean", mass_g = 3 }
    },
    steps = {
      { technique = "pan_sear",       temp_c = 178, duration_sec = 260, flip_midway = false },
      { technique = "whisk_emulsify", temp_c = 82,  duration_sec = 90,  flip_midway = false }
    }
  },
  {
    id = "creme_brulee", name = "Vanilla Bean Creme Brulee", category = "confection",
    notes = "Gentle 82°C bake sets yolk custard without curdling, finished with radiant caramelized sugar crust.",
    shape = "cylinder", lx_mm = 95, ly_mm = 95, lz_mm = 26,
    items = {
      { id = "heavy_cream", mass_g = 180 }, { id = "egg_yolk", mass_g = 55 },
      { id = "sucrose", mass_g = 42 },      { id = "vanilla_bean", mass_g = 4 }
    },
    steps = {
      { technique = "bake",        temp_c = 105, duration_sec = 1900, flip_midway = false },
      { technique = "broil_grill", temp_c = 265, duration_sec = 75,   flip_midway = false }
    }
  },
  {
    id = "chocolate_fondant", name = "Molten 3D-Core Dark Chocolate Fondant", category = "confection",
    notes = "Timed 195°C bake sets outer cake wall (>78°C) while pasteurizing molten 3D fudge center (~63°C).",
    shape = "cylinder", lx_mm = 65, ly_mm = 65, lz_mm = 36,
    items = {
      { id = "dark_chocolate", mass_g = 85 }, { id = "butter", mass_g = 75 },
      { id = "egg_whole", mass_g = 85 },      { id = "sucrose", mass_g = 55 },
      { id = "pastry_flour", mass_g = 35 },   { id = "sea_salt", mass_g = 0.8 }
    },
    steps = { { technique = "bake", temp_c = 195, duration_sec = 1050, flip_midway = false } }
  },
  {
    id = "lacto_sauerkraut", name = "Lacto-Fermented Cabbage Kraut", category = "ferment",
    notes = "2% NaCl brine selects Lactobacillus to convert cabbage sugars into crisp lactic acid tang.",
    shape = "slab", lx_mm = 80, ly_mm = 80, lz_mm = 80,
    items = { { id = "cabbage", mass_g = 300 }, { id = "sea_salt", mass_g = 6.2 } },
    steps = { { technique = "ferment_proof", temp_c = 22, duration_sec = 14400, flip_midway = false, lid_closed = true } }
  }
}

for _, r in ipairs(recipes) do
  fatduck.add_recipe(r)
end
