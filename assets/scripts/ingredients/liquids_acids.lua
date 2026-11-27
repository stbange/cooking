-- 12 Culinary Liquids, Stocks, Wines, Vinegars & Citrus Acids
local items = {
  { id = "water",           name = "Filtered Water",      category = "liquid", water = 1.00, ph = 7.0, shape = "liquid", diet = { "vegan", "gluten_free" }, roles = { "liquid_base" } },
  { id = "chicken_stock",   name = "Roasted Chicken Stock",category="stock",  water = 0.95, protein = 0.032, fat = 0.008, sugar = 0.004, ash = 0.006, collagen_frac = 0.45, ph = 6.2, sodium_mg_100g = 140, glutamate_mg_100g = 85, nucleotides_mg_100g = 45, aroma_potency = 0.45, shape = "liquid", diet = { "gluten_free", "dairy_free" }, roles = { "liquid_base", "umami" } },
  { id = "beef_stock",      name = "Rich Beef Bone Stock",category = "stock",  water = 0.94, protein = 0.040, fat = 0.010, sugar = 0.004, ash = 0.006, collagen_frac = 0.55, ph = 6.0, sodium_mg_100g = 155, glutamate_mg_100g = 95, nucleotides_mg_100g = 55, aroma_potency = 0.55, shape = "liquid", diet = { "gluten_free", "dairy_free" }, roles = { "liquid_base", "umami" } },
  { id = "dashi_stock",     name = "Kombu-Bonito Dashi",  category = "stock",  water = 0.98, protein = 0.012, fat = 0.001, ash = 0.007, ph = 6.1, sodium_mg_100g = 180, glutamate_mg_100g = 160, nucleotides_mg_100g = 130, aroma_potency = 0.55, shape = "liquid", diet = { "gluten_free", "dairy_free" }, roles = { "liquid_base", "umami" } },
  { id = "coconut_milk",    name = "Full-Fat Coconut Milk",category="liquid",  water = 0.74, protein = 0.023, fat = 0.21, sugar = 0.02,  fiber = 0.005, ash = 0.007, ph = 6.2, emulsifier_power = 0.35, aroma_potency = 0.50, shape = "liquid", diet = { "vegan", "gluten_free" }, roles = { "liquid_base", "fat_medium", "emulsifier" } },
  { id = "white_wine",      name = "Dry White Wine",      category = "wine",   water = 0.87, sugar = 0.008, ash = 0.003, ph = 3.3, acidity_molar = 0.09, aroma_potency = 0.65, shape = "liquid", diet = { "vegan", "gluten_free" }, roles = { "liquid_base", "acid", "aromatic" } },
  { id = "red_wine",        name = "Burgundy Red Wine",   category = "wine",   water = 0.86, sugar = 0.006, ash = 0.004, ph = 3.5, acidity_molar = 0.08, bitter_base = 0.22, aroma_potency = 0.72, shape = "liquid", diet = { "vegan", "gluten_free" }, roles = { "liquid_base", "acid", "aromatic" } },
  { id = "lemon_juice",     name = "Fresh Lemon Juice",   category = "acid",   water = 0.92, protein = 0.004, sugar = 0.025, ash = 0.003, ph = 2.3, acidity_molar = 0.32, aroma_potency = 0.65, shape = "liquid", diet = { "vegan", "gluten_free" }, roles = { "acid", "aromatic" } },
  { id = "lime_juice",      name = "Fresh Lime Juice",    category = "acid",   water = 0.91, protein = 0.004, sugar = 0.017, ash = 0.003, ph = 2.2, acidity_molar = 0.35, aroma_potency = 0.70, shape = "liquid", diet = { "vegan", "gluten_free" }, roles = { "acid", "aromatic" } },
  { id = "white_wine_vin",  name = "White Wine Vinegar",  category = "acid",   water = 0.94, sugar = 0.004, ph = 2.6, acidity_molar = 0.85, aroma_potency = 0.45, shape = "liquid", diet = { "vegan", "gluten_free" }, roles = { "acid" } },
  { id = "rice_vinegar",    name = "Seasoned Rice Vinegar",category="acid",    water = 0.93, sugar = 0.02,  ph = 2.9, acidity_molar = 0.65, aroma_potency = 0.40, shape = "liquid", diet = { "vegan", "gluten_free" }, roles = { "acid" } },
  { id = "balsamic_vinegar",name = "Aged Balsamic Vinegar",category="acid",    water = 0.77, sugar = 0.15,  reducing_sugar_frac = 0.90, ph = 3.1, acidity_molar = 0.75, glutamate_mg_100g = 40, aroma_potency = 0.78, shape = "liquid", diet = { "vegan", "gluten_free" }, roles = { "acid", "sweetener", "aromatic" } }
}

for _, it in ipairs(items) do
  fatduck.add_ingredient(it)
end
