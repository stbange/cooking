-- Example User Mod: Adds fermented Black Garlic, a custom recipe, and live scoring hooks!
fatduck.add_ingredient({
  id = "black_garlic",
  name = "Aged Black Garlic (Mod)",
  category = "umami",
  water = 0.52, protein = 0.08, fat = 0.006, starch = 0.08, sugar = 0.24, fiber = 0.03, ash = 0.02,
  reducing_sugar_frac = 0.95, ph = 4.3, acidity_molar = 0.08,
  glutamate_mg_100g = 640, free_amino = 0.90, aroma_potency = 0.95,
  shape = "cube", lx_mm = 12, ly_mm = 12, lz_mm = 12,
  diet = { "vegan", "gluten_free" },
  roles = { "umami", "sweetener", "aromatic" }
})

fatduck.add_recipe({
  id = "black_garlic_miso_salmon",
  name = "Black Garlic & Miso Glazed Salmon (Mod)",
  category = "mod",
  notes = "Demonstrates live Lua extensibility: high reducing sugars + free amines accelerate Maillard glaze.",
  shape = "tapered", lx_mm = 120, ly_mm = 65, lz_mm = 24,
  items = {
    { id = "salmon_fillet", mass_g = 190 },
    { id = "white_miso",    mass_g = 18 },
    { id = "black_garlic",  mass_g = 14 },
    { id = "canola_oil",    mass_g = 8 }
  },
  steps = {
    { technique = "pan_sear",        temp_c = 185, duration_sec = 180, flip_midway = false },
    { technique = "convection_roast",temp_c = 185, duration_sec = 395, flip_midway = false }
  }
})

fatduck.register_mod({
  name = "FatDuck Precision Gastronomy Bonus",
  on_init = function()
    -- Runs automatically on startup and F5 reload
  end,
  on_step_post = function(ctx)
    -- Inspects ctx.elapsed_sec, ctx.core_temp_c, ctx.surf_temp_c
    local _ = ctx.core_temp_c
  end,
  on_score_dish = function(m)
    -- Reward dishes combining crisp exterior (>5) with tender interior (>7) and synergistic umami (>6)
    if m.crispness > 5.0 and m.tenderness > 7.0 and m.umami > 6.0 then
      return 2.5
    end
    return 0.0
  end
})
