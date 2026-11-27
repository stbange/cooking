-- 11 Classic Emulsions, Mother Sauces & Collagen Braises (Multi-Vessel Executable Workflows)
local recipes = {
  {
    id = "classic_hollandaise", name = "Classic Hollandaise Emulsion", category = "emulsion",
    notes = "2-vessel bain-marie emulsion: warm butter melted in Saucepan drizzled under high shear into whisked yolks + lemon.",
    shape = "liquid", lx_mm = 120, ly_mm = 120, lz_mm = 15,
    vessels = {
      { id = "bowl", name = "Stainless Bain-Marie Bowl", kind = "bowl", shape = "liquid", lx_mm = 120, ly_mm = 120, lz_mm = 15 },
      { id = "pot",  name = "Warm Butter Saucepan",      kind = "pot",  shape = "liquid", lx_mm = 80,  ly_mm = 80,  lz_mm = 25 }
    },
    items = {
      { id = "egg_yolk",    mass_g = 54, vessel = "bowl", prep = "3 fresh yolks" },
      { id = "lemon_juice", mass_g = 16, vessel = "bowl", prep = "strained lemon juice" },
      { id = "water",       mass_g = 20, vessel = "bowl", prep = "continuous aqueous phase" },
      { id = "sea_salt",    mass_g = 1.5,vessel = "bowl", prep = "fine sea salt" },
      { id = "butter",      mass_g = 60, vessel = "bowl", prep = "initial warm butter drizzle" },
      { id = "butter",      mass_g = 60, vessel = "pot",  prep = "remaining melted butter" }
    },
    steps = {
      { vessel = "bowl", action = "cook",    technique = "whisk_emulsify", temp_c = 66, duration_sec = 220,
        instruction = "Over gentle 66°C steam bath, whisk 54g egg yolks, 20g water, 16g lemon juice & first 60g warm butter continuously to ribbon stage.",
        why = "Holds yolks between 62°C pasteurization onset and 72.5°C irreversible curdling while lecithin coats droplets." },
      { vessel = "pot",  action = "cook",    technique = "simmer_poach",   temp_c = 62, duration_sec = 120, parallel = true,
        instruction = "Meanwhile keep remaining 60g butter melted at 62°C in Warm Butter Saucepan.",
        why = "Matching butter temperature (~62°C) prevents cold fat seizing or hot >80°C fat scrambling yolks." },
      { vessel = "pot",  action = "combine", target_vessel = "bowl",       temp_c = 65, duration_sec = 140, technique = "whisk_emulsify", agitation = 0.90,
        instruction = "Stream remaining 60g warm butter slowly into Bain-Marie Bowl while whisking vigorously at 65°C for 140s.",
        why = "High shear breaks dispersed fat below 5 um within Krieger-Dougherty packing limit (phi < phi_max)." }
    }
  },
  {
    id = "classic_mayonnaise", name = "Dijon Mayonnaise", category = "emulsion",
    notes = "Cold high-internal-phase emulsion: whisked yolk + Dijon + acid base with slow oil drizzle.",
    shape = "liquid", lx_mm = 80, ly_mm = 80, lz_mm = 40,
    vessels = {
      { id = "bowl", name = "Heavy Mixing Bowl", kind = "bowl", shape = "liquid", lx_mm = 80, ly_mm = 80, lz_mm = 40 },
      { id = "jug",  name = "Oil Pouring Jug",   kind = "bowl", shape = "liquid", lx_mm = 60, ly_mm = 60, lz_mm = 50 }
    },
    items = {
      { id = "egg_yolk",       mass_g = 22,  vessel = "bowl", prep = "room-temp yolk" },
      { id = "dijon_mustard",  mass_g = 12,  vessel = "bowl", prep = "mucilage co-emulsifier" },
      { id = "lemon_juice",    mass_g = 18,  vessel = "bowl", prep = "acidification pH<=4.1" },
      { id = "white_wine_vin", mass_g = 8,   vessel = "bowl", prep = "acetic acid safety" },
      { id = "sea_salt",       mass_g = 1.8, vessel = "bowl", prep = "dissolved in acid" },
      { id = "canola_oil",     mass_g = 150, vessel = "jug",  prep = "slow thin stream" }
    },
    steps = {
      { vessel = "bowl", action = "whisk",   temp_c = 22, duration_sec = 60,  agitation = 0.85,
        instruction = "Whisk 22g egg yolk, 12g Dijon mustard, 18g lemon juice, 8g vinegar & 1.8g salt in Heavy Mixing Bowl until frothy.",
        why = "Hydrates mustard mucilage and lecithin in the continuous water phase before oil arrives." },
      { vessel = "jug",  action = "combine", target_vessel = "bowl", temp_c = 22, duration_sec = 360, technique = "whisk_emulsify", agitation = 0.95,
        instruction = "Drizzle 150g canola oil drop-by-drop at first, then in a thin thread while whisking non-stop for 6 min at 22°C.",
        why = "Crowded oil droplets jam near phi ~ 0.78, rocketing Krieger-Dougherty viscosity into spoon-standing mayo while pH <= 4.15 ensures safety." }
    }
  },
  {
    id = "provencal_aioli", name = "Mortar-Pounded Garlic Olive Oil Aioli", category = "emulsion",
    notes = "Crushed garlic fructans + acidified egg yolk emulsify extra-virgin olive oil cold.",
    shape = "liquid", lx_mm = 80, ly_mm = 80, lz_mm = 40,
    vessels = {
      { id = "mortar", name = "Marble Mortar & Pestle", kind = "bowl", shape = "cube",   lx_mm = 12, ly_mm = 12, lz_mm = 12 },
      { id = "bowl",   name = "Emulsion Bowl",          kind = "bowl", shape = "liquid", lx_mm = 80, ly_mm = 80, lz_mm = 40 }
    },
    items = {
      { id = "garlic",         mass_g = 18,  vessel = "mortar", prep = "peeled cloves" },
      { id = "sea_salt",       mass_g = 1.6, vessel = "mortar", prep = "abrasive coarse salt" },
      { id = "egg_yolk",       mass_g = 20,  vessel = "bowl",   prep = "room-temp yolk" },
      { id = "lemon_juice",    mass_g = 22,  vessel = "bowl",   prep = "fresh lemon" },
      { id = "white_wine_vin", mass_g = 8,   vessel = "bowl",   prep = "wine vinegar" },
      { id = "olive_oil",      mass_g = 130, vessel = "bowl",   add_step = 3, prep = "slow drizzle" }
    },
    steps = {
      { vessel = "mortar", action = "cut",     cut_lx_mm = 1.5, cut_ly_mm = 1.5, cut_lz_mm = 1.5, temp_c = 22, duration_sec = 90,
        instruction = "Crush 18g garlic with 1.6g coarse salt in Marble Mortar into a smooth paste.",
        why = "100% cellular rupture releases fructan polysaccharides and pungent allicin." },
      { vessel = "mortar", action = "combine", target_vessel = "bowl", temp_c = 22, duration_sec = 20,
        instruction = "Scrape garlic paste into Emulsion Bowl with 20g yolk, 22g lemon juice & 8g vinegar.",
        why = "Combines plant fructan + phospholipid surfactants in acid buffer." },
      { vessel = "bowl",   action = "whisk",   temp_c = 22, duration_sec = 380, agitation = 0.90,
        instruction = "Whisk in 130g olive oil in a slow steady thread for ~6 min until thick golden aioli forms.",
        why = "Sustained shear stabilizes olive oil droplets while pH <= 4.15 inactivates microbes cold." }
    }
  },
  {
    id = "beurre_blanc", name = "Shallot White-Wine Beurre Blanc", category = "emulsion",
    notes = "Shallot-wine gastrique reduced in Saucepan, then whisked off-boiling at 58°C with cold cubed butter.",
    shape = "liquid", lx_mm = 80, ly_mm = 80, lz_mm = 35,
    vessels = {
      { id = "pot", name = "Heavy Reduction Pan", kind = "pot", shape = "liquid", lx_mm = 80, ly_mm = 80, lz_mm = 35 }
    },
    items = {
      { id = "white_wine",     mass_g = 50,  vessel = "pot", prep = "dry Muscadet" },
      { id = "white_wine_vin", mass_g = 18,  vessel = "pot", prep = "wine vinegar" },
      { id = "shallot",        mass_g = 25,  vessel = "pot", prep = "fine 2mm mince" },
      { id = "heavy_cream",    mass_g = 20,  vessel = "pot", add_step = 2, prep = "thermal stabilizer" },
      { id = "butter",         mass_g = 115, vessel = "pot", add_step = 3, prep = "cold 12mm cubes" },
      { id = "sea_salt",       mass_g = 1.4, vessel = "pot", add_step = 3, prep = "finishing salt" }
    },
    steps = {
      { vessel = "pot", action = "cook",  technique = "deglaze_reduce", temp_c = 96, duration_sec = 180,
        instruction = "Simmer 50g white wine, 18g vinegar & 25g minced shallot at 96°C for 3 min until reduced to ~2 tbsp glaze.",
        why = "Concentrates tartaric/acetic acid and sweet shallot aromatics." },
      { vessel = "pot", action = "cook",  technique = "simmer_poach",   temp_c = 85, duration_sec = 45,
        instruction = "Stir in 20g heavy cream and warm 45s, then lower heat to 58°C.",
        why = "Cream casein & whey proteins reinforce the delicate butter-in-water film." },
      { vessel = "pot", action = "whisk", technique = "whisk_emulsify", temp_c = 58, duration_sec = 150, agitation = 0.88,
        instruction = "Whisk in 115g cold cubed butter a few cubes at a time at 58°C until velvety and opaque; season with 1.4g salt.",
        why = "Cold butter cubes quench pan enthalpy near 58°C so milk-fat globule membranes stay intact instead of splitting into oil." }
    }
  },
  {
    id = "veloute_roux_sauce", name = "Golden Chicken Veloute", category = "sauce",
    notes = "Blond butter-flour roux cooked in Sauteuse, then whisked with warm chicken stock from a second pot.",
    shape = "liquid", lx_mm = 100, ly_mm = 100, lz_mm = 60,
    vessels = {
      { id = "roux_pan",  name = "Roux Sauteuse",  kind = "skillet", shape = "liquid", lx_mm = 100, ly_mm = 100, lz_mm = 20 },
      { id = "stock_pot", name = "Warm Stockpot",  kind = "pot",     shape = "liquid", lx_mm = 100, ly_mm = 100, lz_mm = 60 }
    },
    items = {
      { id = "butter",        mass_g = 25,  vessel = "roux_pan",  prep = "clarified by foaming" },
      { id = "ap_flour",      mass_g = 25,  vessel = "roux_pan",  prep = "sifted wheat flour" },
      { id = "chicken_stock", mass_g = 260, vessel = "stock_pot", prep = "clear chicken stock" },
      { id = "sea_salt",      mass_g = 1.8, vessel = "stock_pot", prep = "to taste" }
    },
    steps = {
      { vessel = "roux_pan",  action = "cook",    technique = "saute_stirfry", temp_c = 135, duration_sec = 120,
        instruction = "Melt 25g butter in Roux Sauteuse; stir in 25g flour and cook at 135°C for 2 min to a nutty blond roux.",
        why = "Coating starch granules in fat prevents clumping when liquid hits and cooks out raw flour taste." },
      { vessel = "stock_pot", action = "cook",    technique = "simmer_poach",  temp_c = 82,  duration_sec = 120, parallel = true,
        instruction = "Warm 260g chicken stock + 1.8g salt to 82°C in Warm Stockpot.",
        why = "Warm stock disperses hot roux smoothly without thermal lumps." },
      { vessel = "stock_pot", action = "combine", target_vessel = "roux_pan",  temp_c = 90,  duration_sec = 480, technique = "simmer_poach", agitation = 0.75,
        instruction = "Ladle warm stock into blond roux while whisking, then simmer gently at 90°C for 8 min until glossy velouté coats a spoon.",
        why = "Swelling wheat starch granules (>82°C) build shear-thinning viscosity and heat-proof emulsion stability." }
    }
  },
  {
    id = "cacio_e_pepe", name = "Roman Cacio e Pepe", category = "emulsion",
    notes = "3-station workflow: boiling pasta in Pot 1, toasting black pepper in Skillet 2, and Pecorino-starch paste in Bowl 3.",
    shape = "cylinder", lx_mm = 90, ly_mm = 12, lz_mm = 12,
    vessels = {
      { id = "pot",     name = "Pasta Pot (Low Water)", kind = "pot",     shape = "cylinder", lx_mm = 90, ly_mm = 12, lz_mm = 12 },
      { id = "skillet", name = "Pepper Skillet",        kind = "skillet", shape = "liquid",   lx_mm = 90, ly_mm = 90, lz_mm = 15 },
      { id = "bowl",    name = "Pecorino Paste Bowl",   kind = "bowl",    shape = "liquid",   lx_mm = 70, ly_mm = 70, lz_mm = 25 }
    },
    items = {
      { id = "pasta_semolina", mass_g = 110, vessel = "pot",     prep = "bronze-die tonnarelli" },
      { id = "water",          mass_g = 160, vessel = "pot",     prep = "concentrated starch water" },
      { id = "black_pepper",   mass_g = 3,   vessel = "skillet", prep = "freshly cracked" },
      { id = "pecorino",       mass_g = 55,  vessel = "bowl",    prep = "microplane-grated Romano" },
      { id = "water",          mass_g = 30,  vessel = "bowl",    prep = "warm starchy ladleful" }
    },
    steps = {
      { vessel = "pot",     action = "cook",    technique = "rolling_boil",   temp_c = 100, duration_sec = 500,
        instruction = "Boil 110g bronze-die pasta in minimum water (160g retained) at 100°C for 8 min 20s until al dente.",
        why = "Low water ratio concentrates leached amylose starch (>1.5%) needed to shield cheese casein." },
      { vessel = "skillet", action = "cook",    technique = "pan_sear",       temp_c = 155, duration_sec = 45,  parallel = true,
        instruction = "Toast 3g cracked black pepper dry in Pepper Skillet at 155°C for 45s until citrusy piperine blooms, then cool pan.",
        why = "Dry heat volatilizes rotundone & piperine terpenes before liquid is added." },
      { vessel = "bowl",    action = "whisk",   temp_c = 45, duration_sec = 45, agitation = 0.80, parallel = true,
        instruction = "Whisk 55g finely grated Pecorino with 30g warm (~45°C) starchy water into a smooth paste.",
        why = "Pre-hydrating casein below 65°C prevents gritty clumps." },
      { vessel = "skillet", action = "combine", target_vessel = "pot",        temp_c = 78, duration_sec = 10,
        instruction = "Toss toasted pepper into drained al dente pasta off-heat and let cool 20s toward ~68°C.",
        why = "Dropping pasta below 70°C prevents Pecorino proteins from seizing into stringy curds." },
      { vessel = "bowl",    action = "combine", target_vessel = "pot",        temp_c = 64, duration_sec = 80, technique = "whisk_emulsify", agitation = 0.85,
        instruction = "Vigorously toss Pecorino paste with pasta at 64°C for 80s until a glossy, creamy emulsion coats every strand.",
        why = "Gelatinized pasta starch sterically stabilizes melted sheep-milk fat and casein." }
    }
  },
  {
    id = "pasta_carbonara", name = "Traditional Roman Carbonara", category = "emulsion",
    notes = "True 3-vessel Roman technique: Pasta Stockpot + Guanciale Skillet + Yolk-Pecorino Whisk Bowl tempered off-heat at 65°C.",
    shape = "cylinder", lx_mm = 90, ly_mm = 12, lz_mm = 12,
    vessels = {
      { id = "pot",     name = "Salted Pasta Pot",       kind = "pot",     shape = "cylinder", lx_mm = 90, ly_mm = 12, lz_mm = 12 },
      { id = "skillet", name = "Guanciale Skillet",      kind = "skillet", shape = "cube",     lx_mm = 12, ly_mm = 10, lz_mm = 8 },
      { id = "bowl",    name = "Yolk & Pecorino Bowl",   kind = "bowl",    shape = "liquid",   lx_mm = 75, ly_mm = 75, lz_mm = 30 }
    },
    items = {
      { id = "pasta_semolina", mass_g = 110, vessel = "pot",     prep = "rigatoni or spaghettoni" },
      { id = "water",          mass_g = 180, vessel = "pot",     prep = "starchy cooking water" },
      { id = "pork_belly",     mass_g = 55,  vessel = "skillet", prep = "10mm guanciale lardons" },
      { id = "black_pepper",   mass_g = 2.5, vessel = "skillet", add_step = 2, prep = "coarse cracked pepper" },
      { id = "egg_yolk",       mass_g = 40,  vessel = "bowl",    prep = "2 large yolks" },
      { id = "pecorino",       mass_g = 35,  vessel = "bowl",    prep = "finely grated Pecorino" }
    },
    steps = {
      { vessel = "pot",     action = "cook",    technique = "rolling_boil",   temp_c = 100, duration_sec = 520,
        instruction = "Boil 110g pasta in salted water at 100°C for 8 min 40s until al dente, reserving starchy water.",
        why = "Gelatinizes semolina starch (~85%) while keeping firm al dente core." },
      { vessel = "skillet", action = "cook",    technique = "pan_sear",       temp_c = 175, duration_sec = 240, flip_midway = true, parallel = true,
        instruction = "In parallel, render 55g pork lardons in Guanciale Skillet at 175°C for 4 min until golden-crisp; bloom 2.5g pepper & turn burner OFF.",
        why = "Renders clear pork fat and crisp Maillard edges; turning heat off prevents scrambling yolks in Step 5." },
      { vessel = "bowl",    action = "whisk",   temp_c = 22, duration_sec = 60, agitation = 0.80, parallel = true,
        instruction = "In Yolk & Pecorino Bowl, whisk 40g egg yolks with 35g grated Pecorino into a thick golden paste.",
        why = "Disperses yolk lecithin with salty sheep-milk protein before heat exposure." },
      { vessel = "skillet", action = "combine", target_vessel = "pot",        temp_c = 76, duration_sec = 25, agitation = 0.65,
        instruction = "Toss rendered pork + warm liquid fat with drained pasta & starchy water; let equilibrate 25s to ~74°C.",
        why = "Starch water + liquid pork fat form pre-emulsion and cool pasta into the safe tempering window." },
      { vessel = "bowl",    action = "combine", target_vessel = "pot",        temp_c = 67, duration_sec = 165, technique = "whisk_emulsify", agitation = 0.85, flip_midway = true, lid_closed = true,
        instruction = "Pour yolk-Pecorino paste over pasta over gentle 67°C bain-marie steam (never direct burner!), tossing briskly ~2.5 min until glossy.",
        why = "T_mix ~ 66-67°C achieves >=6.5D yolk pasteurization and custard thickening without crossing 73°C scramble threshold!" }
    }
  },
  {
    id = "boeuf_bourguignon", name = "Boeuf Bourguignon Collagen Braise", category = "braise",
    notes = "3-station French classic: Prep Board cubing -> Cast-Iron searing & Pinot Noir fond deglaze -> Dutch Oven 2h 89°C braise.",
    shape = "cube", lx_mm = 42, ly_mm = 42, lz_mm = 42,
    vessels = {
      { id = "board",   name = "Butcher Prep Board", kind = "board",   shape = "slab", lx_mm = 120, ly_mm = 90, lz_mm = 45 },
      { id = "skillet", name = "Cast-Iron Skillet",  kind = "skillet", shape = "cube", lx_mm = 42,  ly_mm = 42, lz_mm = 42 },
      { id = "cocotte", name = "Enameled Dutch Oven",kind = "pot",     shape = "cube", lx_mm = 42,  ly_mm = 42, lz_mm = 42 }
    },
    items = {
      { id = "beef_chuck",       mass_g = 280, vessel = "board",   prep = "well-marbled chuck roast" },
      { id = "sea_salt",         mass_g = 3.2, vessel = "board",   prep = "seasoning salt" },
      { id = "red_wine",         mass_g = 120, vessel = "skillet", add_step = 4, prep = "Burgundy Pinot Noir" },
      { id = "beef_stock",       mass_g = 120, vessel = "cocotte", prep = "rich bone stock" },
      { id = "carrot",           mass_g = 50,  vessel = "cocotte", prep = "25mm oblique chunks" },
      { id = "yellow_onion",     mass_g = 50,  vessel = "cocotte", prep = "diced mirepoix" },
      { id = "cremini_mushroom", mass_g = 50,  vessel = "cocotte", prep = "quartered caps" },
      { id = "tomato_paste",     mass_g = 15,  vessel = "cocotte", prep = "pincé umami paste" }
    },
    steps = {
      { vessel = "board",   action = "cut",     cut_lx_mm = 42, cut_ly_mm = 42, cut_lz_mm = 42, cut_shape = "cube", duration_sec = 75,
        instruction = "Cut 280g beef chuck into 42mm cubes on Butcher Prep Board, pat dry, and season with 3.2g salt.",
        why = "Substantial 42mm cubes retain interior succulence across a 2-hour collagen braise." },
      { vessel = "board",   action = "combine", target_vessel = "skillet", temp_c = 20,  duration_sec = 10,
        instruction = "Arrange seasoned chuck cubes in a single uncrowded layer in preheated Cast-Iron Skillet.",
        why = "Uncrowded spacing lets steam escape so pan stays >180°C instead of steaming meat gray." },
      { vessel = "skillet", action = "cook",    technique = "pan_sear",    temp_c = 205, duration_sec = 180, flip_midway = true,
        instruction = "Brown chuck cubes hard at 205°C for 3 min, tossing halfway to crust multiple faces and build dark pan fond.",
        why = "Generates hundreds of savory Maillard melanoidins & pyrazines that gentle braising at 89°C never could." },
      { vessel = "skillet", action = "deglaze", target_vessel = "cocotte", temp_c = 94,  duration_sec = 90,
        instruction = "Pour 120g Pinot Noir into hot skillet, scrape every speck of browned fond loose for 90s, and transfer all into Dutch Oven.",
        why = "Water + ethanol dissolve pan-bound Maillard fond and carry its umami directly into the braising liquor." },
      { vessel = "cocotte", action = "cook",    technique = "braise",      temp_c = 89,  duration_sec = 7200, lid_closed = true,
        instruction = "Cover Dutch Oven with stock, carrot, onion, mushrooms & tomato paste; braise gently at 89°C (never a hard boil) for 2 hours.",
        why = "Sub-boiling 89°C hydrolyzes triple-helix collagen (>68°C) into silky mouth-coating gelatin without squeezing muscle fibers dry." }
    }
  },
  {
    id = "braised_pork_belly", name = "Soy-Caramel Braised Pork Belly (Hong Shao Rou)", category = "braise",
    notes = "2-vessel workflow: caramel-ginger glaze in Wok combined into Claypot for slow fat-cap collagen hydrolysis.",
    shape = "fat_cap", lx_mm = 80, ly_mm = 65, lz_mm = 36,
    vessels = {
      { id = "wok",     name = "Seasoned Wok",    kind = "skillet", shape = "fat_cap", lx_mm = 80, ly_mm = 65, lz_mm = 36 },
      { id = "claypot", name = "Covered Claypot", kind = "pot",     shape = "fat_cap", lx_mm = 80, ly_mm = 65, lz_mm = 36 }
    },
    items = {
      { id = "pork_belly",    mass_g = 260, vessel = "wok",     prep = "36mm skin-on blocks" },
      { id = "brown_sugar",   mass_g = 18,  vessel = "wok",     prep = "muscovado caramel base" },
      { id = "fresh_ginger",  mass_g = 10,  vessel = "wok",     prep = "smashed coins" },
      { id = "soy_sauce",     mass_g = 32,  vessel = "claypot", prep = "dark + light soy" },
      { id = "chicken_stock", mass_g = 140, vessel = "claypot", prep = "unsalted stock" }
    },
    steps = {
      { vessel = "wok",     action = "cook",    technique = "pan_sear", temp_c = 190, duration_sec = 180, flip_midway = true,
        instruction = "Sear 260g pork belly with 18g brown sugar & ginger in Wok at 190°C for 3 min until amber caramel coats skin.",
        why = "Simultaneous sucrose caramelization + amino-soy Maillard creates glossy mahogany color." },
      { vessel = "wok",     action = "combine", target_vessel = "claypot", temp_c = 90, duration_sec = 15,
        instruction = "Transfer caramelized pork belly and rendered aromatics into Covered Claypot with soy sauce & stock.",
        why = "Claypot thermal mass stabilizes gentle sub-simmer." },
      { vessel = "claypot", action = "cook",    technique = "braise",   temp_c = 90,  duration_sec = 6600, lid_closed = true,
        instruction = "Braise covered at 90°C for 1 hr 50 min until skin & connective tissue quiver like custard.",
        why = "Converts skin collagen into gelatin so fat cap feels silky rather than greasy." }
    }
  },
  {
    id = "french_onion_soup", name = "Caramelized French Onion Gratin", category = "braise",
    notes = "Sliced onions caramelized 30 min in Dutch Oven, deglazed with white wine, simmered in bone stock & gratineed.",
    shape = "slab", lx_mm = 90, ly_mm = 90, lz_mm = 30,
    vessels = {
      { id = "board", name = "Prep Board",       kind = "board", shape = "slab", lx_mm = 90, ly_mm = 90, lz_mm = 60 },
      { id = "pot",   name = "Heavy Dutch Oven", kind = "pot",   shape = "slab", lx_mm = 90, ly_mm = 90, lz_mm = 30 }
    },
    items = {
      { id = "yellow_onion", mass_g = 300, vessel = "board", prep = "peeled yellow onions" },
      { id = "butter",       mass_g = 28,  vessel = "pot",   prep = "sweet butter" },
      { id = "sea_salt",     mass_g = 2.4, vessel = "pot",   prep = "osmotic sweat salt" },
      { id = "white_wine",   mass_g = 40,  vessel = "pot",   add_step = 4, prep = "dry white wine" },
      { id = "beef_stock",   mass_g = 220, vessel = "pot",   add_step = 5, prep = "roasted bone stock" },
      { id = "gruyere",      mass_g = 45,  vessel = "pot",   add_step = 5, prep = "aged Gruyere cap" }
    },
    steps = {
      { vessel = "board", action = "cut",     cut_lx_mm = 90, cut_ly_mm = 6, cut_lz_mm = 30, cut_shape = "slab", duration_sec = 60,
        instruction = "Slice 300g yellow onions pole-to-pole into even 4-6mm ribbons on Prep Board.",
        why = "Pole-to-pole slicing keeps vascular fibers intact so ribbons melt jammy instead of disintegrating." },
      { vessel = "board", action = "combine", target_vessel = "pot", temp_c = 25, duration_sec = 10,
        instruction = "Toss sliced onions into Heavy Dutch Oven with 28g melted butter and 2.4g salt.",
        why = "Salt draws vacuole water out osmotically for rapid initial softening." },
      { vessel = "pot",   action = "cook",    technique = "saute_stirfry", temp_c = 155, duration_sec = 1800, flip_midway = true,
        instruction = "Cook onions at 155°C for 30 min, stirring repeatedly until deep mahogany fond coats the pot.",
        why = "Onion fructans hydrolyze into fructose (~110°C caramelization) alongside amino Maillard." },
      { vessel = "pot",   action = "deglaze", temp_c = 95, duration_sec = 90,
        instruction = "Deglaze dark onion fond with 40g dry white wine, scraping pot floor clean.",
        why = "Captures concentrated caramelized fond into the broth." },
      { vessel = "pot",   action = "cook",    technique = "simmer_poach",  temp_c = 90, duration_sec = 1110,
        instruction = "Add 220g beef stock, simmer 18 min, and melt 45g Gruyere on top.",
        why = "Aged Gruyere glutamate + beef nucleotides amplify savory depth." }
    }
  },
  {
    id = "umami_bone_jus", name = "Roasted Beef & Shiitake Umami Jus", category = "sauce",
    notes = "Pincé tomato-shallot fond in Skillet deglazed into simmering beef-shiitake stockpot for synergistic umami.",
    shape = "liquid", lx_mm = 100, ly_mm = 100, lz_mm = 50,
    vessels = {
      { id = "skillet", name = "Saute Skillet", kind = "skillet", shape = "liquid", lx_mm = 90,  ly_mm = 90,  lz_mm = 20 },
      { id = "pot",     name = "Reduction Pot", kind = "pot",     shape = "liquid", lx_mm = 100, ly_mm = 100, lz_mm = 50 }
    },
    items = {
      { id = "shallot",      mass_g = 25,  vessel = "skillet", prep = "sliced shallots" },
      { id = "tomato_paste", mass_g = 18,  vessel = "skillet", prep = "double-concentrated" },
      { id = "butter",       mass_g = 20,  vessel = "skillet", prep = "roasting butter" },
      { id = "beef_stock",   mass_g = 300, vessel = "pot",     prep = "collagen bone stock" },
      { id = "shiitake",     mass_g = 45,  vessel = "pot",     prep = "5'-GMP mushroom caps" },
      { id = "soy_sauce",    mass_g = 10,  vessel = "pot",     prep = "free glutamate boost" }
    },
    steps = {
      { vessel = "skillet", action = "cook",    technique = "saute_stirfry", temp_c = 165, duration_sec = 180,
        instruction = "Caramelize 25g shallot & 18g tomato paste (pincé) in 20g butter at 165°C until brick-red.",
        why = "Roasting tomato fructose + free glutamate builds deep savory fond without raw metallic acidity." },
      { vessel = "skillet", action = "combine", target_vessel = "pot",       temp_c = 92,  duration_sec = 15,
        instruction = "Scrape brick-red tomato-shallot fond into Reduction Pot with 300g beef stock, 45g shiitake & 10g soy.",
        why = "Combines beef 5'-IMP, shiitake 5'-GMP, and tomato/soy L-glutamate in one reduction." },
      { vessel = "pot",     action = "cook",    technique = "simmer_poach",  temp_c = 94,  duration_sec = 1620,
        instruction = "Simmer uncovered at 94°C for 27 min to reduce and concentrate into a glossy nappe jus.",
        why = "Multiplicative umami synergy (1 + gamma*[IMP+GMP]) peaks as water evaporates." }
    }
  }
}

for _, r in ipairs(recipes) do
  fatduck.add_recipe(r)
end
