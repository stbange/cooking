-- 11 Classic Starches, Glycemic Benchmarks, Baking, Fermentation & Sugar Chemistry Multi-Vessel Recipes
local recipes = {
  {
    id = "pasta_pomodoro", name = "Pasta Al Dente al Pomodoro", category = "starch",
    notes = "2-pot Italian workflow: Stockpot boiling pasta al dente while Skillet simmers garlic-olive-oil tomato sugo.",
    shape = "cylinder", lx_mm = 90, ly_mm = 10, lz_mm = 10,
    vessels = {
      { id = "pot",     name = "Salted Pasta Stockpot", kind = "pot",     shape = "cylinder", lx_mm = 90, ly_mm = 10, lz_mm = 10 },
      { id = "skillet", name = "Wide Sugo Skillet",     kind = "skillet", shape = "liquid",   lx_mm = 100,ly_mm = 100,lz_mm = 25 }
    },
    items = {
      { id = "pasta_semolina", mass_g = 110, vessel = "pot",     prep = "durum spaghetti" },
      { id = "water",          mass_g = 200, vessel = "pot",     prep = "salted boiling water" },
      { id = "tomato",         mass_g = 160, vessel = "skillet", prep = "crushed San Marzano" },
      { id = "olive_oil",      mass_g = 18,  vessel = "skillet", prep = "extra-virgin olive oil" },
      { id = "garlic",         mass_g = 8,   vessel = "skillet", prep = "sliced cloves" },
      { id = "sea_salt",       mass_g = 2.2, vessel = "skillet", prep = "to balance acidity" }
    },
    steps = {
      { vessel = "pot",     action = "cook",    technique = "rolling_boil",  temp_c = 100, duration_sec = 510,
        instruction = "Boil 110g durum spaghetti at 100°C for 8.5 min until exterior starch gelatinizes while core stays al dente.",
        why = "Al dente core + semolina protein network moderates digestive amylase access (lower eGI)." },
      { vessel = "skillet", action = "cook",    technique = "saute_stirfry", temp_c = 125, duration_sec = 360, parallel = true,
        instruction = "Meanwhile bloom 8g sliced garlic in 18g olive oil and simmer 160g crushed tomatoes + 2.2g salt in Sugo Skillet for 6 min.",
        why = "Heat ruptures tomato cells, releasing pectin and oil-soluble lycopene." },
      { vessel = "skillet", action = "combine", target_vessel = "pot",       temp_c = 88,  duration_sec = 45, technique = "simmer_poach", agitation = 0.70,
        instruction = "Toss al dente pasta directly with simmering tomato sugo & olive oil for 45s to emulsify starch and pectin.",
        why = "Mechanical tossing marries amylose starch with tomato pectin + olive oil into a clinging gloss." }
    }
  },
  {
    id = "mushroom_risotto", name = "Creamy Shiitake Parmigiano Risotto", category = "starch",
    notes = "3-vessel northern Italian workflow: seared shiitakes, hot stockpot ladle additions, and cold-butter mantecatura.",
    shape = "liquid", lx_mm = 110, ly_mm = 110, lz_mm = 45,
    vessels = {
      { id = "risotto", name = "Heavy Risotto Casserole", kind = "pot",     shape = "liquid", lx_mm = 110, ly_mm = 110, lz_mm = 45 },
      { id = "shroom",  name = "Mushroom Skillet",        kind = "skillet", shape = "slab",   lx_mm = 50,  ly_mm = 50,  lz_mm = 10 }
    },
    items = {
      { id = "arborio_rice",  mass_g = 95,  vessel = "risotto", prep = "high-amylopectin carnaroli/arborio" },
      { id = "white_wine",    mass_g = 35,  vessel = "risotto", prep = "dry white wine" },
      { id = "chicken_stock", mass_g = 280, vessel = "risotto", prep = "hot simmering stock" },
      { id = "shiitake",      mass_g = 60,  vessel = "shroom",  prep = "8mm sliced caps" },
      { id = "butter",        mass_g = 24,  vessel = "risotto", add_step = 4, prep = "cold cubed mantecatura" },
      { id = "parmesan",      mass_g = 30,  vessel = "risotto", add_step = 4, prep = "aged 24-mo Parmigiano" }
    },
    steps = {
      { vessel = "risotto", action = "cook",    technique = "simmer_poach",  temp_c = 92,  duration_sec = 1080,
        instruction = "Toast 95g Arborio rice, deglaze with 35g wine, and simmer with 280g hot stock at 92°C for 18 min while stirring.",
        why = "Gentle agitation rubs swollen amylopectin off grain exteriors into a natural starch emulsion." },
      { vessel = "shroom",  action = "cook",    technique = "pan_sear",      temp_c = 195, duration_sec = 210, flip_midway = true, parallel = true,
        instruction = "In Mushroom Skillet, sear 60g sliced shiitake at 195°C for 3.5 min until golden.",
        why = "Separate dry sear develops nutty Maillard crust instead of rubbery boiled mushrooms." },
      { vessel = "shroom",  action = "combine", target_vessel = "risotto",   temp_c = 78,  duration_sec = 15,
        instruction = "Fold browned shiitakes into risotto and pull casserole off the burner.",
        why = "Drops risotto toward ~72°C before dairy addition." },
      { vessel = "risotto", action = "whisk",   technique = "whisk_emulsify",temp_c = 68,  duration_sec = 60, agitation = 0.85,
        instruction = "Beat in 24g cold butter + 30g Parmigiano off-heat (mantecatura) for 60s until risotto flows all'onda.",
        why = "High-shear beating suspends cold butter droplets inside amylopectin + casein gel." }
    }
  },
  {
    id = "triple_cooked_chips", name = "Fat Duck Triple-Cooked Chips", category = "starch",
    notes = "Heston Blumenthal 3-vessel benchmark: Prep Board batons -> Simmer Pot fissuring -> Deep-Fryer 135°C blanch & 190°C crisp!",
    shape = "cube", lx_mm = 65, ly_mm = 16, lz_mm = 16,
    vessels = {
      { id = "board", name = "Prep Board",        kind = "board", shape = "slab", lx_mm = 110, ly_mm = 75, lz_mm = 65 },
      { id = "pot",   name = "Simmering Pot",     kind = "pot",   shape = "cube", lx_mm = 65,  ly_mm = 16, lz_mm = 16 },
      { id = "fryer", name = "190°C Deep-Fryer",  kind = "pot",   shape = "cube", lx_mm = 65,  ly_mm = 16, lz_mm = 16 }
    },
    items = {
      { id = "russet_potato", mass_g = 240, vessel = "board", prep = "high-solids russet" },
      { id = "canola_oil",    mass_g = 30,  vessel = "fryer", prep = "clean frying oil" },
      { id = "sea_salt",      mass_g = 2.8, vessel = "fryer", add_step = 5, prep = "fine flaky salt" }
    },
    steps = {
      { vessel = "board", action = "cut",     cut_lx_mm = 65, cut_ly_mm = 16, cut_lz_mm = 16, cut_shape = "cube", duration_sec = 60,
        instruction = "Cut 240g Russet potato into crisp 65x16x16mm batons and rinse off loose surface starch.",
        why = "16mm square cross-section maximizes 2-axis crispy edges while preserving fluffy interior." },
      { vessel = "board", action = "combine", target_vessel = "pot",   temp_c = 25,  duration_sec = 5,
        instruction = "Slip rinsed batons gently into Simmering Pot.",
        why = "Starts even hydration from outside in." },
      { vessel = "pot",   action = "cook",    technique = "simmer_poach", temp_c = 94, duration_sec = 720,
        instruction = "Simmer batons gently at 94°C for 12 min until edges just begin to micro-fissure, then drain & steam-dry.",
        why = "Gelatinizes starch and creates microscopic surface cracks that become glass-crisp pockets." },
      { vessel = "pot",   action = "combine", target_vessel = "fryer", temp_c = 135, duration_sec = 300, technique = "deep_fry",
        instruction = "Blanch-fry dried batons in Deep-Fryer at 135°C for 5 min to set a pale rigid starch crust without browning.",
        why = "Drives moisture out of outer 2mm shell before Maillard temperatures are applied." },
      { vessel = "fryer", action = "cook",    technique = "deep_fry",     temp_c = 190, duration_sec = 210, flip_midway = true,
        instruction = "Blast at 190°C for 3.5 min, tossing halfway until deep golden-amber, and season immediately with 2.8g salt.",
        why = "Remaining capillary water flashes violently to steam, locking low-aw glass crispness around a cloud-soft core." }
    }
  },
  {
    id = "pommes_puree", name = "Robuchon Butter Pommes Puree", category = "starch",
    notes = "Yukon potatoes simmered in Pot 1, riced & beaten with cold cubed butter + hot milk warmed in Pot 2.",
    shape = "cube", lx_mm = 32, ly_mm = 32, lz_mm = 32,
    vessels = {
      { id = "pot",      name = "Potato Stockpot", kind = "pot", shape = "cube",   lx_mm = 32, ly_mm = 32, lz_mm = 32 },
      { id = "milk_pan", name = "Hot Milk Pan",    kind = "pot", shape = "liquid", lx_mm = 65, ly_mm = 65, lz_mm = 25 }
    },
    items = {
      { id = "yukon_potato", mass_g = 250, vessel = "pot",      prep = "even 32mm chunks" },
      { id = "sea_salt",     mass_g = 2.8, vessel = "pot",      prep = "seasoning salt" },
      { id = "butter",       mass_g = 85,  vessel = "pot",      add_step = 3, prep = "cold cubed butter" },
      { id = "whole_milk",   mass_g = 60,  vessel = "milk_pan", prep = "scalded whole milk" }
    },
    steps = {
      { vessel = "pot",      action = "cook",    technique = "simmer_poach",   temp_c = 92, duration_sec = 1260,
        instruction = "Simmer 250g Yukon potato chunks at 92°C (never a violent boil) for 21 min until knife-tender; drain & steam dry.",
        why = "Gentle 92°C gelatinizes starch granules intact without rupturing cells into gluey free amylose." },
      { vessel = "milk_pan", action = "cook",    technique = "simmer_poach",   temp_c = 85, duration_sec = 120, parallel = true,
        instruction = "Meanwhile heat 60g whole milk to 85°C in Hot Milk Pan.",
        why = "Hot milk absorbs smoothly into riced potato without cooling the emulsion." },
      { vessel = "pot",      action = "fold",    temp_c = 76, duration_sec = 60, agitation = 0.35,
        instruction = "Pass hot potatoes through fine ricer and fold in 85g cold cubed butter gently (low shear!) until absorbed.",
        why = "Coating intact starch cells in butter fat first waterproofs them against gluey over-hydration." },
      { vessel = "milk_pan", action = "combine", target_vessel = "pot",        temp_c = 74, duration_sec = 45, agitation = 0.40,
        instruction = "Stream 85°C milk into buttered potato puree, stirring briskly just until satin-smooth.",
        why = "Forms a velvety starch-dairy emulsion with high hot eGI benchmark." }
    }
  },
  {
    id = "chilled_potato_salad_rs3", name = "Chilled Vinaigrette Potato Salad (RS3 Low-GI)", category = "starch",
    notes = "2-vessel glycemic benchmark: simmered Yukon cubes dressed warm with whisked Dijon vinaigrette, then chilled at 4°C for RS3.",
    shape = "cube", lx_mm = 26, ly_mm = 26, lz_mm = 26, chill_after = true,
    vessels = {
      { id = "pot",  name = "Simmer Saucepan",  kind = "pot",  shape = "cube",   lx_mm = 26, ly_mm = 26, lz_mm = 26 },
      { id = "bowl", name = "Vinaigrette Bowl", kind = "bowl", shape = "liquid", lx_mm = 75, ly_mm = 75, lz_mm = 30 }
    },
    items = {
      { id = "yukon_potato",   mass_g = 240, vessel = "pot",  prep = "26mm cubes" },
      { id = "sea_salt",       mass_g = 2.4, vessel = "pot",  prep = "in cooking water" },
      { id = "olive_oil",      mass_g = 22,  vessel = "bowl", prep = "extra-virgin olive oil" },
      { id = "white_wine_vin", mass_g = 18,  vessel = "bowl", prep = "acetic acid GI reducer" },
      { id = "dijon_mustard",  mass_g = 10,  vessel = "bowl", prep = "emulsifier" },
      { id = "shallot",        mass_g = 15,  vessel = "bowl", prep = "finely minced" }
    },
    steps = {
      { vessel = "pot",  action = "cook",    technique = "simmer_poach", temp_c = 93, duration_sec = 1200,
        instruction = "Simmer 240g 26mm Yukon potato cubes in salted water at 93°C for 20 min until tender; drain.",
        why = "Full starch gelatinization is the required precursor before cold RS3 recrystallization." },
      { vessel = "bowl", action = "whisk",   temp_c = 22, duration_sec = 60, agitation = 0.85, parallel = true,
        instruction = "In Vinaigrette Bowl, whisk 18g white wine vinegar, 10g Dijon, 15g minced shallot & 22g olive oil until emulsified.",
        why = "Mustard mucilage stabilizes oil-vinegar dressing." },
      { vessel = "bowl", action = "combine", target_vessel = "pot",      temp_c = 55, duration_sec = 30, agitation = 0.30,
        instruction = "Fold Dijon vinaigrette gently over warm potatoes, then refrigerate at 4°C for 4 hours before serving.",
        why = "4°C chill nucleates retrograded RS3 resistant starch while acetic acid slows gastric emptying (markedly lower eGI!)." }
    }
  },
  {
    id = "sourdough_boule", name = "Artisanal Sourdough Country Boule", category = "baking",
    notes = "Kneaded & stretch-folded in Mixing Bowl to build gluten network, fermented at 27°C, then baked in covered & open Dutch Oven.",
    shape = "sphere", lx_mm = 165, ly_mm = 165, lz_mm = 95,
    vessels = {
      { id = "bowl",    name = "Dough Proofing Bowl", kind = "bowl", shape = "sphere", lx_mm = 165, ly_mm = 165, lz_mm = 95 },
      { id = "cocotte", name = "Preheated Dutch Oven",kind = "oven", shape = "sphere", lx_mm = 165, ly_mm = 165, lz_mm = 95 }
    },
    items = {
      { id = "bread_flour",       mass_g = 300, vessel = "bowl", prep = "12.7% protein bread flour" },
      { id = "water",             mass_g = 215, vessel = "bowl", prep = "72% hydration warm water" },
      { id = "sourdough_starter", mass_g = 65,  vessel = "bowl", prep = "active levain" },
      { id = "sea_salt",          mass_g = 6.2, vessel = "bowl", prep = "2% baker's salt" }
    },
    steps = {
      { vessel = "bowl",    action = "knead",   temp_c = 26,  duration_sec = 300, agitation = 0.85,
        instruction = "Mix flour, water, levain & salt in Dough Bowl; slap-and-fold / knead 5 min until smooth windowpane gluten forms.",
        why = "Mechanical shear aligns hydrated gliadin + glutenin into gas-tight viscoelastic sheets." },
      { vessel = "bowl",    action = "cook",    technique = "ferment_proof", temp_c = 27,  duration_sec = 7200, lid_closed = true,
        instruction = "Bulk ferment & proof covered at 27°C for 2 hours until airy, domed, and tangy.",
        why = "Wild yeast inflates CO2 porosity inside the gluten network while Lactobacillus lowers pH." },
      { vessel = "bowl",    action = "combine", target_vessel = "cocotte",   temp_c = 125, duration_sec = 10,   agitation = 0.20,
        instruction = "Score boule cleanly and transfer gently into Preheated Dutch Oven without deflating gas.",
        why = "Gentle transfer preserves fermented gas cells for maximum oven spring." },
      { vessel = "cocotte", action = "cook",    technique = "braise",        temp_c = 125, duration_sec = 1500, lid_closed = true,
        instruction = "Bake covered in steamy Dutch Oven for 25 min so loaf expands freely before crust sets.",
        why = "Trapped steam keeps surface pliable during 55-82°C thermal oven-spring expansion." },
      { vessel = "cocotte", action = "cook",    technique = "bake",          temp_c = 195, duration_sec = 900,  lid_closed = false,
        instruction = "Remove lid and bake uncovered at 195°C for 15 min until blistered mahogany crust sings.",
        why = "Dry radiant heat drives rapid crust Maillard and low-aw crackle." }
    }
  },
  {
    id = "brioche_roll", name = "Enriched Butter Brioche", category = "baking",
    notes = "Kneaded wheat gluten network enriched with egg & butter, proofed at 29°C, and baked golden.",
    shape = "sphere", lx_mm = 95, ly_mm = 95, lz_mm = 70,
    vessels = {
      { id = "mixer", name = "Stand Mixer Bowl", kind = "bowl", shape = "sphere", lx_mm = 95, ly_mm = 95, lz_mm = 70 },
      { id = "oven",  name = "Deck Baking Tin",  kind = "oven", shape = "sphere", lx_mm = 95, ly_mm = 95, lz_mm = 70 }
    },
    items = {
      { id = "bread_flour", mass_g = 180, vessel = "mixer", prep = "strong flour" },
      { id = "egg_whole",   mass_g = 80,  vessel = "mixer", prep = "chilled eggs" },
      { id = "whole_milk",  mass_g = 35,  vessel = "mixer", prep = "milk" },
      { id = "sucrose",     mass_g = 22,  vessel = "mixer", prep = "caster sugar" },
      { id = "dry_yeast",   mass_g = 4,   vessel = "mixer", prep = "instant yeast" },
      { id = "sea_salt",    mass_g = 3.2, vessel = "mixer", prep = "fine salt" },
      { id = "butter",      mass_g = 75,  vessel = "mixer", add_step = 2, prep = "softened pliable butter" }
    },
    steps = {
      { vessel = "mixer", action = "knead",   temp_c = 24,  duration_sec = 360, agitation = 0.90,
        instruction = "Knead flour, eggs, milk, sugar, yeast & salt for 6 min BEFORE adding butter so gluten hydrates unimpeded.",
        why = "Adding fat too early coats flour proteins waterproof and stunts gluten network formation!" },
      { vessel = "mixer", action = "cook",    technique = "ferment_proof", temp_c = 29,  duration_sec = 5400, lid_closed = true,
        instruction = "Work in 75g softened butter, then proof covered at 29°C for 90 min until doubled.",
        why = "Butter shortens gluten strands into a feather-soft, rich crumb." },
      { vessel = "mixer", action = "combine", target_vessel = "oven",      temp_c = 29,  duration_sec = 10,   agitation = 0.25,
        instruction = "Shape gently into Deck Baking Tin and brush with egg wash.",
        why = "Preserves delicate fermentation bubbles." },
      { vessel = "oven",  action = "cook",    technique = "bake",          temp_c = 185, duration_sec = 1320,
        instruction = "Bake at 185°C for 22 min until internal crumb sets >88°C and crust glows amber.",
        why = "Higher sugar + egg amino content browns rapidly at 185°C without burning." }
    }
  },
  {
    id = "salted_caramel_sauce", name = "Salted Amber Caramel Sauce", category = "confection",
    notes = "Dry sucrose caramelized at 178°C in Heavy Saucepan, quenched & whisked with pre-warmed cream from Pan 2.",
    shape = "liquid", lx_mm = 90, ly_mm = 90, lz_mm = 30,
    vessels = {
      { id = "sugar_pan", name = "Heavy Copper Saucepan", kind = "pot", shape = "liquid", lx_mm = 90, ly_mm = 90, lz_mm = 30 },
      { id = "cream_pan", name = "Warm Cream Pan",        kind = "pot", shape = "liquid", lx_mm = 70, ly_mm = 70, lz_mm = 25 }
    },
    items = {
      { id = "sucrose",      mass_g = 140, vessel = "sugar_pan", prep = "dry white sugar" },
      { id = "heavy_cream",  mass_g = 100, vessel = "cream_pan", prep = "warmed cream" },
      { id = "vanilla_bean", mass_g = 3,   vessel = "cream_pan", prep = "scraped pod" },
      { id = "butter",       mass_g = 40,  vessel = "sugar_pan", add_step = 3, prep = "cubed butter" },
      { id = "sea_salt",     mass_g = 2.5, vessel = "sugar_pan", add_step = 3, prep = "fleur de sel" }
    },
    steps = {
      { vessel = "sugar_pan", action = "cook",    technique = "pan_sear",       temp_c = 178, duration_sec = 260,
        instruction = "Melt 140g sucrose in Heavy Copper Saucepan at 178°C for 4 min 20s until deep amber and fragrant.",
        why = "Sucrose thermal pyrolysis above 165°C generates nutty furanones and butterscotch diacetyl." },
      { vessel = "cream_pan", action = "cook",    technique = "simmer_poach",   temp_c = 80,  duration_sec = 90, parallel = true,
        instruction = "Meanwhile warm 100g heavy cream + vanilla bean to 80°C in Warm Cream Pan.",
        why = "Pre-warming cream prevents 175°C sugar from seizing into a solid glass rock upon contact!" },
      { vessel = "cream_pan", action = "combine", target_vessel = "sugar_pan",  temp_c = 82,  duration_sec = 90, technique = "whisk_emulsify", agitation = 0.85,
        instruction = "Carefully stream 80°C cream into amber caramel off-heat, then whisk in 40g butter & 2.5g fleur de sel until glossy.",
        why = "Quenches caramelization instantly while emulsifying dairy fat into liquid sugar." }
    }
  },
  {
    id = "creme_brulee", name = "Vanilla Bean Creme Brulee", category = "confection",
    notes = "Infused vanilla cream in Saucepan tempered into whisked yolks in Bowl, baked gently in Ramekin & torched.",
    shape = "cylinder", lx_mm = 95, ly_mm = 95, lz_mm = 26,
    vessels = {
      { id = "pan",     name = "Infusion Saucepan", kind = "pot",  shape = "liquid",   lx_mm = 85, ly_mm = 85, lz_mm = 30 },
      { id = "bowl",    name = "Custard Bowl",      kind = "bowl", shape = "liquid",   lx_mm = 85, ly_mm = 85, lz_mm = 30 },
      { id = "ramekin", name = "Porcelain Ramekin", kind = "oven", shape = "cylinder", lx_mm = 95, ly_mm = 95, lz_mm = 26 }
    },
    items = {
      { id = "heavy_cream",  mass_g = 180, vessel = "pan",     prep = "36% heavy cream" },
      { id = "vanilla_bean", mass_g = 4,   vessel = "pan",     prep = "split & scraped" },
      { id = "egg_yolk",     mass_g = 55,  vessel = "bowl",    prep = "4 yolks" },
      { id = "sucrose",      mass_g = 32,  vessel = "bowl",    prep = "custard sugar" },
      { id = "sucrose",      mass_g = 10,  vessel = "ramekin", add_step = 5, prep = "torch crust sugar" }
    },
    steps = {
      { vessel = "pan",     action = "cook",    technique = "simmer_poach", temp_c = 76,  duration_sec = 180,
        instruction = "Steep 180g heavy cream + split vanilla bean at 76°C for 3 min.",
        why = "Extracts fat-soluble vanillin into cream droplets." },
      { vessel = "bowl",    action = "whisk",   temp_c = 22,  duration_sec = 45,  agitation = 0.60, parallel = true,
        instruction = "Whisk 55g egg yolks + 32g sugar smoothly in Custard Bowl without whipping excess foam.",
        why = "Dissolves sucrose immediately so osmotic yolk burn ('cooked specks') cannot form." },
      { vessel = "pan",     action = "combine", target_vessel = "bowl",     temp_c = 58,  duration_sec = 20,  agitation = 0.50,
        instruction = "Stream warm 76°C vanilla cream slowly into yolks while stirring to temper near 58°C, then strain into Ramekin.",
        why = "Gradual enthalpy equilibration raises yolk temperature gently without local curdling." },
      { vessel = "bowl",    action = "combine", target_vessel = "ramekin",  temp_c = 105, duration_sec = 1850, technique = "bake",
        instruction = "Bake Ramekin in water-bath deck at 105°C for ~31 min until custard trembles like silk (~79°C).",
        why = "Sucrose elevates yolk coagulation threshold from 70°C to ~79°C for quivering texture." },
      { vessel = "ramekin", action = "cook",    technique = "broil_grill",  temp_c = 265, duration_sec = 75,
        instruction = "Sprinkle 10g sugar evenly on top and caramelize under 265°C radiant heat for 75s into a crackly amber glass.",
        why = "Intense radiant flux melts & caramelizes top 1mm sugar before heat conducts into cold custard." }
    }
  },
  {
    id = "chocolate_fondant", name = "Molten 3D-Core Dark Chocolate Fondant", category = "confection",
    notes = "3-vessel pastry physics: melted chocolate-butter + whipped egg-sugar ribbon foam + gentle porosity-preserving fold & 195°C bake!",
    shape = "cylinder", lx_mm = 65, ly_mm = 65, lz_mm = 36,
    vessels = {
      { id = "melt",    name = "Chocolate Bain-Marie", kind = "pot",  shape = "liquid",   lx_mm = 80, ly_mm = 80, lz_mm = 25 },
      { id = "whip",    name = "Egg Whipping Bowl",    kind = "bowl", shape = "liquid",   lx_mm = 90, ly_mm = 90, lz_mm = 40 },
      { id = "ramekin", name = "Buttered Ramekin",     kind = "oven", shape = "cylinder", lx_mm = 65, ly_mm = 65, lz_mm = 36 }
    },
    items = {
      { id = "dark_chocolate", mass_g = 85, vessel = "melt",    prep = "70% cacao pistoles" },
      { id = "butter",         mass_g = 75, vessel = "melt",    prep = "cubed unsalted butter" },
      { id = "egg_whole",      mass_g = 85, vessel = "whip",    prep = "room-temp eggs" },
      { id = "sucrose",        mass_g = 55, vessel = "whip",    prep = "fine caster sugar" },
      { id = "sea_salt",       mass_g = 0.8,vessel = "whip",    prep = "pinch salt" },
      { id = "pastry_flour",   mass_g = 35, vessel = "ramekin", add_step = 4, prep = "sifted soft flour" }
    },
    steps = {
      { vessel = "melt",    action = "cook",    technique = "simmer_poach", temp_c = 54, duration_sec = 180,
        instruction = "Melt 85g dark chocolate + 75g butter over 54°C water bath until glossy and smooth.",
        why = "Melts cocoa butter crystals cleanly below 58°C where chocolate seizes or scorches." },
      { vessel = "whip",    action = "whisk",   temp_c = 24, duration_sec = 180, agitation = 0.95, parallel = true,
        instruction = "Whip 85g whole eggs, 55g sugar & salt at high speed for 3 min until pale, tripled in volume, and holding a thick ribbon.",
        why = "Incorporates micro-air foam porosity (sole leavening!) stabilized by unfolded ovalbumin + viscous sugar syrup." },
      { vessel = "melt",    action = "fold",    target_vessel = "whip", temp_c = 38, duration_sec = 45, agitation = 0.22,
        instruction = "Gently fold warm chocolate-butter into whipped egg foam with a wide spatula using slow J-strokes.",
        why = "Low-shear folding (agitation <= 0.25) preserves >90% of delicate whipped air bubbles vs harsh stirring deflating them!" },
      { vessel = "whip",    action = "combine", target_vessel = "ramekin", temp_c = 34, duration_sec = 30, agitation = 0.22,
        instruction = "Sift 35g pastry flour over batter, fold gently in 8 strokes, and pour into Buttered Ramekin.",
        why = "Minimal folding avoids developing tough gluten while keeping airy mousse density." },
      { vessel = "ramekin", action = "cook",    technique = "bake", temp_c = 185, duration_sec = 750,
        instruction = "Bake at 185°C for 12.5 min so outer 10mm sets into souffléed cake (>78°C) while 3D core stays molten (~64°C pasteurized).",
        why = "3D radial thermal lag + insulating foam porosity creates a self-saucing molten chocolate center." }
    }
  },
  {
    id = "lacto_sauerkraut", name = "Lacto-Fermented Cabbage Kraut", category = "ferment",
    notes = "Finely shredded & salt-massaged cabbage on Prep Board packed into Anaerobic Crock for Lactobacillus souring.",
    shape = "slab", lx_mm = 80, ly_mm = 80, lz_mm = 80,
    vessels = {
      { id = "board", name = "Shredding Board", kind = "board", shape = "slab", lx_mm = 120, ly_mm = 120, lz_mm = 90 },
      { id = "crock", name = "Stoneware Crock", kind = "pot",   shape = "slab", lx_mm = 80,  ly_mm = 80,  lz_mm = 80 }
    },
    items = {
      { id = "cabbage",  mass_g = 300, vessel = "board", prep = "cored green cabbage" },
      { id = "sea_salt", mass_g = 6.2, vessel = "board", prep = "2.05% non-iodized salt" }
    },
    steps = {
      { vessel = "board", action = "cut",     cut_lx_mm = 80, cut_ly_mm = 3, cut_lz_mm = 80, duration_sec = 120,
        instruction = "Shred 300g cabbage into 3mm ribbons on Shredding Board and massage vigorously with 6.2g sea salt until self-brining.",
        why = "3mm cuts + mechanical bruising rupture vacuoles so osmotic 2% NaCl draws out submerged anaerobic brine." },
      { vessel = "board", action = "combine", target_vessel = "crock", temp_c = 22, duration_sec = 20,
        instruction = "Pack salted cabbage and extracted brine tightly under weights in Stoneware Crock.",
        why = "Submerging beneath 2% brine excludes aerobic spoilage molds while selecting halotolerant LAB." },
      { vessel = "crock", action = "cook",    technique = "ferment_proof", temp_c = 22, duration_sec = 14400, lid_closed = true,
        instruction = "Ferment covered at 22°C as Lactobacillus converts plant sugars into crisp lactic acid tang.",
        why = "Lactic acid drops aqueous pH toward ~3.9, preserving crunch and bright sour complexity." }
    }
  }
}

for _, r in ipairs(recipes) do
  fatduck.add_recipe(r)
end
