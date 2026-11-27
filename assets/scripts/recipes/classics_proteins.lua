-- 12 Classic Benchmark Protein, Seafood, Egg & Tofu Multi-Vessel Executable Recipes
local recipes = {
  {
    id = "reverse_sear_ribeye", name = "Reverse-Sear Ribeye & Red-Wine Shallot Jus", category = "protein",
    notes = "3-station workflow: 135°C oven roast -> 225°C cast-iron Maillard sear -> parallel red-wine fond jus & 5-min carryover rest.",
    shape = "bone_in", lx_mm = 110, ly_mm = 85, lz_mm = 32,
    vessels = {
      { id = "skillet", name = "Cast-Iron Skillet & Oven Rack", kind = "skillet", shape = "bone_in", lx_mm = 110, ly_mm = 85, lz_mm = 32 },
      { id = "saucepan", name = "Small Jus Saucepan",           kind = "pot",     shape = "liquid",  lx_mm = 80,  ly_mm = 80, lz_mm = 30 },
      { id = "plate",    name = "Warm Carving Board",           kind = "plate",   shape = "bone_in", lx_mm = 110, ly_mm = 85, lz_mm = 32 }
    },
    items = {
      { id = "beef_ribeye",      mass_g = 320, vessel = "skillet",  prep = "32mm bone-in, patted dry" },
      { id = "clarified_butter", mass_g = 18,  vessel = "skillet",  prep = "high-smoke-point sear fat" },
      { id = "sea_salt",         mass_g = 3.5, vessel = "skillet",  prep = "1.1% kosher dry-brine" },
      { id = "rosemary",         mass_g = 4,   vessel = "skillet",  add_step = 2, prep = "bruised sprigs" },
      { id = "garlic",           mass_g = 6,   vessel = "skillet",  add_step = 2, prep = "crushed cloves" },
      { id = "shallot",          mass_g = 22,  vessel = "saucepan", prep = "2mm fine brunoise" },
      { id = "red_wine",         mass_g = 45,  vessel = "saucepan", prep = "dry Pinot Noir" },
      { id = "beef_stock",       mass_g = 55,  vessel = "saucepan", prep = "unsalted bone stock" }
    },
    steps = {
      { vessel = "skillet",  action = "cook",    technique = "convection_roast", temp_c = 135, duration_sec = 2100, flip_midway = false,
        instruction = "Season 320g bone-in ribeye (32mm) with 3.5g sea salt; roast on wire rack at 135°C for 35 min until cold-spot near bone reaches ~51°C.",
        why = "Gentle convective air desiccates outer pellicle for instant Maillard while minimizing gray overcooked band." },
      { vessel = "skillet",  action = "cook",    technique = "pan_sear",         temp_c = 225, duration_sec = 120,  flip_midway = true,
        instruction = "Heat 18g clarified butter in smoking-hot 225°C cast-iron skillet; sear steak 60s per side with crushed garlic & rosemary.",
        why = "Dry pellicle crosses 155°C in seconds, driving rapid Maillard pyrazines while depositing savory fond." },
      { vessel = "saucepan", action = "cook",    technique = "deglaze_reduce",   temp_c = 95,  duration_sec = 300,  parallel = true,
        instruction = "In parallel, sweat 22g minced shallot in saucepan and reduce 45g red wine + 55g beef stock by half into glossy jus.",
        why = "Concentrates glutamate + wine tartaric acid to balance rich ribeye lipid coating." },
      { vessel = "skillet",  action = "combine", target_vessel = "plate",        temp_c = 28,  duration_sec = 5,
        instruction = "Transfer seared ribeye onto warm carving board, leaving rendered burnt solids behind.",
        why = "Halts conductive 225°C bottom heat immediately so actin does not contract above 62°C." },
      { vessel = "plate",    action = "rest",    technique = "rest_hold",        temp_c = 28,  duration_sec = 300,
        instruction = "Rest ribeye uncovered for 5 min as core equilibrates +2.5°C and muscle fibers reabsorb juices.",
        why = "Cooling myofibrils widen and re-trap interstitial moisture, cutting carving purge loss in half." },
      { vessel = "saucepan", action = "combine", target_vessel = "plate",        temp_c = 68,  duration_sec = 15,   agitation = 0.35,
        instruction = "Spoon 68°C reduced red-wine shallot jus over rested ribeye; slice 8mm thick against the grain.",
        why = "Marries glossy umami reduction onto crisp crust right at serving." }
    }
  },
  {
    id = "steak_au_poivre_complete", name = "Complete Steak au Poivre, Cognac Jus & Crispy Potatoes", category = "protein",
    notes = "Full 4-vessel bistro service: cubed potatoes in Skillet 2, peppered tenderloin in Skillet 1, pan-fond cream reduction, and rested plating.",
    shape = "cylinder", lx_mm = 78, ly_mm = 78, lz_mm = 36,
    vessels = {
      { id = "board",   name = "Prep Board",           kind = "board",   shape = "cube",     lx_mm = 50, ly_mm = 50, lz_mm = 50 },
      { id = "pot_pan", name = "Potato Sauteuse",      kind = "skillet", shape = "cube",     lx_mm = 22, ly_mm = 22, lz_mm = 22 },
      { id = "steak",   name = "Heavy Cast-Iron Pan",  kind = "skillet", shape = "cylinder", lx_mm = 78, ly_mm = 78, lz_mm = 36 },
      { id = "plate",   name = "Warm Bistro Plate",    kind = "plate",   shape = "cylinder", lx_mm = 78, ly_mm = 78, lz_mm = 36 }
    },
    items = {
      { id = "yukon_potato",     mass_g = 150, vessel = "board",   prep = "peeled Yukon gold" },
      { id = "clarified_butter", mass_g = 16,  vessel = "pot_pan", prep = "pan-roasting fat" },
      { id = "beef_tenderloin",  mass_g = 220, vessel = "steak",   prep = "36mm center-cut filet" },
      { id = "black_pepper",     mass_g = 3.5, vessel = "steak",   prep = "coarse cracked peppercorns" },
      { id = "sea_salt",         mass_g = 3.2, vessel = "steak",   prep = "flaky salt" },
      { id = "butter",           mass_g = 18,  vessel = "steak",   add_step = 4, prep = "basting butter" },
      { id = "beef_stock",       mass_g = 45,  vessel = "steak",   add_step = 7, prep = "deglazing stock" },
      { id = "heavy_cream",      mass_g = 40,  vessel = "steak",   add_step = 7, prep = "36% cream" }
    },
    steps = {
      { vessel = "board",   action = "cut",     cut_lx_mm = 20, cut_ly_mm = 20, cut_lz_mm = 20, cut_shape = "cube", duration_sec = 60,
        instruction = "Cut 150g Yukon potato into uniform 20mm cubes on the prep board and pat dry.",
        why = "Uniform 20mm geometry ensures all cubes gelatinize internally at the exact same moment." },
      { vessel = "board",   action = "combine", target_vessel = "pot_pan", temp_c = 20, duration_sec = 10,
        instruction = "Transfer dried 20mm potato cubes into the Potato Sauteuse with 16g clarified butter.",
        why = "Dry starch surface prevents oil splatter and accelerates crisp crust formation." },
      { vessel = "pot_pan", action = "cook",    technique = "saute_stirfry", temp_c = 188, duration_sec = 960, flip_midway = true,
        instruction = "Saute potato cubes at 188°C for 16 min, tossing halfway until 3-axis corners turn golden-crisp.",
        why = "3D corner vertices receive 3-face heat flux, crisping while core starch gelatinizes (>65°C)." },
      { vessel = "steak",   action = "cook",    technique = "pan_sear", temp_c = 205, duration_sec = 240, flip_midway = true, parallel = true,
        instruction = "Meanwhile press 3.5g cracked pepper into 220g tenderloin; sear at 205°C for 4 min, flipping & basting with 18g butter.",
        why = "Builds deep Maillard fond and piperine aroma." },
      { vessel = "steak",   action = "cook",    technique = "convection_roast", temp_c = 185, duration_sec = 720, parallel = true,
        instruction = "Slide peppered tenderloin skillet into 185°C oven for 12 min until 36mm center reaches pasteurized medium-rare (~60°C).",
        why = "Uniform 3D oven convection brings thick 36mm filet core past 6.5D safety without burning cracked pepper." },
      { vessel = "steak",   action = "combine", target_vessel = "plate", temp_c = 28, duration_sec = 210,
        instruction = "Move seared filet to the Warm Bistro Plate to rest while making the pan sauce in the hot skillet.",
        why = "Separates meat from blazing metal so carryover gently finishes the center while fond stays in pan." },
      { vessel = "steak",   action = "deglaze", target_vessel = "plate", temp_c = 92, duration_sec = 90,
        instruction = "Pour 45g beef stock + 40g heavy cream into the hot peppercorn skillet, boil 90s to dissolve fond, and spoon over rested filet.",
        why = "Dissolves pan-bound Maillard melanoidins into a glossy emulsified au poivre sauce." },
      { vessel = "pot_pan", action = "combine", target_vessel = "plate", temp_c = 72, duration_sec = 15, agitation = 0.35,
        instruction = "Plate crisp golden Yukon cubes alongside the sauced tenderloin and serve immediately.",
        why = "Unites crisp 3-axis potato corners with tender rested beef." }
    }
  },
  {
    id = "schnitzel_viennese", name = "Pounded Pork Schnitzel with Browned Lemon Butter", category = "protein",
    notes = "Demonstrates mechanical pounding (22mm -> 7mm) + myofibril tenderization followed by rapid 185°C fry.",
    shape = "slab", lx_mm = 110, ly_mm = 80, lz_mm = 22,
    vessels = {
      { id = "board",   name = "Butcher Prep Board", kind = "board",   shape = "slab",   lx_mm = 110, ly_mm = 80, lz_mm = 22 },
      { id = "fryer",   name = "Wide Frying Pan",    kind = "skillet", shape = "slab",   lx_mm = 195, ly_mm = 140, lz_mm = 7 },
      { id = "butterp", name = "Small Butter Pan",   kind = "pot",     shape = "liquid", lx_mm = 70,  ly_mm = 70, lz_mm = 20 }
    },
    items = {
      { id = "pork_loin",        mass_g = 185, vessel = "board",   prep = "boneless loin cutlet" },
      { id = "ap_flour",         mass_g = 22,  vessel = "board",   prep = "light breading coat" },
      { id = "egg_whole",        mass_g = 25,  vessel = "board",   prep = "beaten egg wash" },
      { id = "sea_salt",         mass_g = 2.2, vessel = "board",   prep = "fine sea salt" },
      { id = "clarified_butter", mass_g = 30,  vessel = "fryer",   prep = "traditional Butterschmalz" },
      { id = "butter",           mass_g = 18,  vessel = "butterp", prep = "whole dairy butter" },
      { id = "lemon_juice",      mass_g = 10,  vessel = "butterp", prep = "fresh squeezed lemon" }
    },
    steps = {
      { vessel = "board",   action = "pound",   cut_lz_mm = 7, duration_sec = 60,
        instruction = "Pound 185g pork loin between parchment from 22mm down to an even 7mm sheet; season & dredge in flour and egg.",
        why = "Reduces Fourier diffusion thickness L by 3.1x (~10x faster core cook) and mechanically disrupts tough myofibrils." },
      { vessel = "board",   action = "combine", target_vessel = "fryer", temp_c = 20, duration_sec = 5,
        instruction = "Lay breaded 7mm cutlet gently into preheated clarified butter in the Wide Frying Pan.",
        why = "Immediate steam release puffs the egg-flour coating into a souffléed crust." },
      { vessel = "fryer",   action = "cook",    technique = "deep_fry", temp_c = 185, duration_sec = 165, flip_midway = true,
        instruction = "Soufflé-fry schnitzel at 185°C for 2 min 45s, swirling hot fat over top and flipping halfway.",
        why = "7mm core hits >6.5D pasteurization in <3 min before actin squeeze dries the lean loin." },
      { vessel = "butterp", action = "cook",    technique = "butter_baste", temp_c = 152, duration_sec = 75, parallel = true,
        instruction = "In Small Butter Pan, foam 18g butter at 152°C until nutty milk solids brown, then quench with 10g lemon juice.",
        why = "Milk casein + lactose undergo rapid Maillard noisette aroma, brightened by citric acid." },
      { vessel = "butterp", action = "combine", target_vessel = "fryer", temp_c = 70, duration_sec = 10, agitation = 0.30,
        instruction = "Drain crisp schnitzel briefly and drizzle warm lemon noisette butter across the golden crust.",
        why = "Preserves crisp low-aw breading while adding bright acid-fat contrast." }
    }
  },
  {
    id = "butter_basted_duck", name = "Crispy-Skin Butter-Basted Duck Breast", category = "protein",
    notes = "Renders subcutaneous fat cap while keeping lean breast rose and juicy via arroser basting and resting.",
    shape = "fat_cap", lx_mm = 120, ly_mm = 75, lz_mm = 26,
    vessels = {
      { id = "skillet", name = "Cold-Start Skillet", kind = "skillet", shape = "fat_cap", lx_mm = 120, ly_mm = 75, lz_mm = 26 },
      { id = "rack",    name = "Warm Resting Rack",  kind = "plate",   shape = "fat_cap", lx_mm = 120, ly_mm = 75, lz_mm = 26 }
    },
    items = {
      { id = "duck_breast", mass_g = 240, vessel = "skillet", prep = "crosshatch-scored fat cap" },
      { id = "sea_salt",    mass_g = 2.6, vessel = "skillet", prep = "flaky salt on skin" },
      { id = "butter",      mass_g = 22,  vessel = "skillet", add_step = 2, prep = "foaming baste butter" },
      { id = "fresh_thyme", mass_g = 3,   vessel = "skillet", add_step = 2, prep = "whole sprigs" }
    },
    steps = {
      { vessel = "skillet", action = "cook",    technique = "pan_sear",         temp_c = 178, duration_sec = 360, flip_midway = false,
        instruction = "Place scored duck breast skin-side down in skillet; render at 178°C for 6 min without flipping.",
        why = "Melts subcutaneous fat cap (>68°C) and desiccates collagen skin into glass-crispness." },
      { vessel = "skillet", action = "cook",    technique = "butter_baste",     temp_c = 165, duration_sec = 180, flip_midway = true,
        instruction = "Add 22g butter + thyme; flip briefly and spoon 165°C foaming butter continuously for 3 min.",
        why = "Convective fat basting cooks top and sides evenly while infusing lipid-soluble thymol." },
      { vessel = "skillet", action = "cook",    technique = "convection_roast", temp_c = 180, duration_sec = 340, flip_midway = false,
        instruction = "Transfer skillet to 180°C oven for 5.5 min until center hits pasteurized medium-rosé.",
        why = "Uniform 3D oven air finishes thick breast core without scorching skin." },
      { vessel = "skillet", action = "combine", target_vessel = "rack",         temp_c = 28,  duration_sec = 10,
        instruction = "Lift duck breast skin-side up onto Warm Resting Rack so steam escapes without softening skin.",
        why = "Prevents trapped bottom condensation from raising skin water activity aw above 0.55." },
      { vessel = "rack",    action = "rest",    technique = "rest_hold",        temp_c = 28,  duration_sec = 240,
        instruction = "Rest 4 min uncovered before slicing bias medallions.",
        why = "Equilibrates 3D thermal gradient and locks rose myoglobin juices inside relaxed fibers." }
    }
  },
  {
    id = "onsen_egg_63c", name = "63°C Heston Onsen Egg in Warm Dashi", category = "egg",
    notes = "Isothermal sous-vide equilibrium in Water Bath paired with gently warmed umami dashi in a second pot.",
    shape = "egg", lx_mm = 56, ly_mm = 44, lz_mm = 44,
    vessels = {
      { id = "bath",  name = "63.5°C Sous-Vide Bath", kind = "pot",  shape = "egg",    lx_mm = 56, ly_mm = 44, lz_mm = 44 },
      { id = "dashi", name = "Small Dashi Pot",       kind = "pot",  shape = "liquid", lx_mm = 70, ly_mm = 70, lz_mm = 25 }
    },
    items = {
      { id = "egg_whole",   mass_g = 60, vessel = "bath",  prep = "whole shell egg" },
      { id = "dashi_stock", mass_g = 25, vessel = "dashi", prep = "kombu-bonito dashi" },
      { id = "soy_sauce",   mass_g = 4,  vessel = "dashi", prep = "usukuchi soy" }
    },
    steps = {
      { vessel = "bath",  action = "cook",    technique = "sous_vide",    temp_c = 63.5, duration_sec = 2700,
        instruction = "Immerse 60g egg in precision 63.5°C water bath for 45 min.",
        why = "Yolk livetin thickens into fudgy custard at 63°C while ovalbumin (~80°C) stays silky-tender." },
      { vessel = "dashi", action = "cook",    technique = "simmer_poach", temp_c = 68,   duration_sec = 180, parallel = true,
        instruction = "Warm 25g dashi + 4g soy sauce to 68°C in Small Dashi Pot without boiling.",
        why = "Preserves delicate bonito volatiles and avoids further coagulating egg white upon plating." },
      { vessel = "dashi", action = "combine", target_vessel = "bath",     temp_c = 63,   duration_sec = 10,
        instruction = "Crack custardy 63°C egg gently into bowl and ladle warm seasoned dashi around it.",
        why = "Synergistic glutamate + inosinate broth frames the velvety yolk." }
    }
  },
  {
    id = "soft_boiled_egg_6m", name = "7-Minute Jammy Pasteurized Egg", category = "egg",
    notes = "Steep radial 3D thermal gradient sets outer albumen at >80°C while pasteurizing jammy yolk core ~63°C.",
    shape = "egg", lx_mm = 52, ly_mm = 40, lz_mm = 40,
    items = { { id = "egg_whole", mass_g = 58 }, { id = "sea_salt", mass_g = 0.6 } },
    steps = {
      { technique = "rolling_boil", temp_c = 100, duration_sec = 460, flip_midway = false,
        instruction = "Lower chilled 58g egg into rolling 100°C boiling water for exactly 7 min 40s, then peel & season with 0.6g flaky salt.",
        why = "Radial spherical conduction fully sets outer albumen while yolk center crosses 6.5D pasteurization at jammy viscosity." }
    }
  },
  {
    id = "french_omelette", name = "Classic Silky French Omelette", category = "egg",
    notes = "Whisked whole eggs in Mixing Bowl cooked with high agitation in buttered skillet for pale custardy curds.",
    shape = "slab", lx_mm = 160, ly_mm = 110, lz_mm = 8,
    vessels = {
      { id = "bowl",    name = "Whisking Bowl",     kind = "bowl",    shape = "liquid", lx_mm = 90,  ly_mm = 90,  lz_mm = 35 },
      { id = "skillet", name = "Nonstick Omelette Pan", kind = "skillet", shape = "slab", lx_mm = 160, ly_mm = 110, lz_mm = 8 }
    },
    items = {
      { id = "egg_whole", mass_g = 150, vessel = "bowl",    prep = "3 room-temp eggs" },
      { id = "sea_salt",  mass_g = 1.6, vessel = "bowl",    prep = "dissolved 5 min ahead" },
      { id = "butter",    mass_g = 20,  vessel = "skillet", prep = "unsalted sweet butter" }
    },
    steps = {
      { vessel = "bowl",    action = "whisk",   temp_c = 22,  duration_sec = 60, agitation = 0.70,
        instruction = "Whisk 150g whole eggs with 1.6g salt in Whisking Bowl for 60s until yolk and albumen are completely homogeneous.",
        why = "NaCl weakens yolk-albumen electrostatic attraction for finer, silkier curds and traps micro-air." },
      { vessel = "bowl",    action = "combine", target_vessel = "skillet", temp_c = 65, duration_sec = 5, agitation = 0.35,
        instruction = "Melt 20g butter in Omelette Pan until foaming subsides, then pour beaten eggs in all at once.",
        why = "Butter water evaporation pins pan near 105°C so bottom sets gently without browning." },
      { vessel = "skillet", action = "cook",    technique = "saute_stirfry", temp_c = 160, duration_sec = 285, flip_midway = true, lid_closed = true,
        instruction = "Stir briskly with spatula while shaking pan to form tiny creamy curds, then roll into a smooth pale cylinder.",
        why = "High shear breaks coagulating protein sheets into micro-curds below Maillard temperature." }
    }
  },
  {
    id = "crispy_skin_salmon", name = "Crispy-Skin Tapered Salmon & Warm Lemon Emulsion", category = "seafood",
    notes = "Skin-side sear on Carbon-Steel Skillet paired with a parallel 58°C lemon-butter emulsion in a saucepan.",
    shape = "tapered", lx_mm = 120, ly_mm = 65, lz_mm = 22,
    vessels = {
      { id = "skillet", name = "Carbon-Steel Skillet", kind = "skillet", shape = "tapered", lx_mm = 120, ly_mm = 65, lz_mm = 22 },
      { id = "sauce",   name = "Small Emulsion Pan",   kind = "pot",     shape = "liquid",  lx_mm = 65,  ly_mm = 65, lz_mm = 22 }
    },
    items = {
      { id = "salmon_fillet", mass_g = 190, vessel = "skillet", prep = "tapered skin-on, scaled & dried" },
      { id = "canola_oil",    mass_g = 8,   vessel = "skillet", prep = "neutral sear oil" },
      { id = "sea_salt",      mass_g = 2.0, vessel = "skillet", prep = "on skin & flesh" },
      { id = "butter",        mass_g = 14,  vessel = "sauce",   prep = "cold cubed butter" },
      { id = "lemon_juice",   mass_g = 8,   vessel = "sauce",   prep = "fresh lemon juice" },
      { id = "water",         mass_g = 10,  vessel = "sauce",   prep = "emulsion base" }
    },
    steps = {
      { vessel = "skillet", action = "cook",    technique = "pan_sear",       temp_c = 198, duration_sec = 340, flip_midway = false,
        instruction = "Press 190g salmon skin-side down into oiled 198°C skillet for 5 min 40s until 80% of heat rises from skin upward.",
        why = "Skin fat/collagen layer insulates delicate flesh while desiccating bottom z=0 into crisp glass." },
      { vessel = "skillet", action = "cook",    technique = "butter_baste",   temp_c = 168, duration_sec = 215, flip_midway = true,
        instruction = "Flip briefly onto flesh side at 168°C for 3.5 min until thick head cold-spot reaches safe coral translucency.",
        why = "Asymmetric 3D tapered fillet requires checking thick head (x < nx/2) rather than thin tail." },
      { vessel = "sauce",   action = "whisk",   technique = "whisk_emulsify", temp_c = 58,  duration_sec = 90,  agitation = 0.80, parallel = true,
        instruction = "In Small Emulsion Pan, whisk 14g cold cubed butter into 8g lemon juice + 10g water at 58°C until glossy.",
        why = "Dairy milk-fat globule phospholipids suspend butter droplets below 65°C phase separation." },
      { vessel = "sauce",   action = "combine", target_vessel = "skillet",    temp_c = 58,  duration_sec = 10,  agitation = 0.30,
        instruction = "Plate salmon skin-side up and spoon warm lemon emulsion around the base (not on top of crisp skin).",
        why = "Keeps upper skin crispness aw low while bathing flaky myotomes in bright citrus fat." }
    }
  },
  {
    id = "garlic_butter_shrimp", name = "Garlic-Butter Seared Shrimp", category = "seafood",
    notes = "Minced garlic on Prep Board added after initial high-heat shrimp sear so allicin perfumes butter without burning.",
    shape = "cylinder", lx_mm = 48, ly_mm = 20, lz_mm = 16,
    vessels = {
      { id = "board",   name = "Prep Board",         kind = "board",   shape = "cube",     lx_mm = 15, ly_mm = 15, lz_mm = 15 },
      { id = "skillet", name = "Hot Saute Skillet",  kind = "skillet", shape = "cylinder", lx_mm = 48, ly_mm = 20, lz_mm = 16 }
    },
    items = {
      { id = "garlic",      mass_g = 10,  vessel = "board",   prep = "peeled cloves" },
      { id = "shrimp",      mass_g = 180, vessel = "skillet", prep = "peeled, patted dry" },
      { id = "butter",      mass_g = 24,  vessel = "skillet", prep = "clarified/sweet butter" },
      { id = "sea_salt",    mass_g = 1.8, vessel = "skillet", prep = "fine salt" },
      { id = "lemon_juice", mass_g = 8,   vessel = "skillet", add_step = 4, prep = "finishing squeeze" }
    },
    steps = {
      { vessel = "board",   action = "cut",     cut_lx_mm = 2, cut_ly_mm = 2, cut_lz_mm = 2, duration_sec = 40,
        instruction = "Finely mince 10g garlic to 2mm brunoise on Prep Board and let sit 40s for alliinase enzyme activation.",
        why = "Cell rupture mixes alliin + alliinase to generate fragrant allicin before heat inactivates the enzyme." },
      { vessel = "skillet", action = "cook",    technique = "saute_stirfry", temp_c = 192, duration_sec = 150, flip_midway = true,
        instruction = "Sear 180g dry shrimp in 24g hot butter at 192°C for 2.5 min, flipping once as astaxanthin turns coral.",
        why = "Builds quick crust before minced garlic could scorch bitter at 192°C." },
      { vessel = "board",   action = "combine", target_vessel = "skillet", temp_c = 165, duration_sec = 5,
        instruction = "Toss 2mm minced garlic into skillet for the final 65s.",
        why = "Late addition blooms sweet nutty garlic Maillard without bitter pyrolysis." },
      { vessel = "skillet", action = "cook",    technique = "saute_stirfry", temp_c = 175, duration_sec = 70,
        instruction = "Add 8g lemon juice, toss 70s until shrimp core crosses 6.5D pasteurization, and pull off heat.",
        why = "Hits safe, snappy tenderness before prolonged actin contraction turns shrimp rubbery." }
    }
  },
  {
    id = "sous_vide_chicken", name = "62°C Pasteurized Sous-Vide Chicken Breast", category = "protein",
    notes = "2-vessel workflow: isothermal 62.5°C Sous-Vide Water Bath pasteurization followed by 60s 210°C Cast-Iron sear.",
    shape = "tapered", lx_mm = 125, ly_mm = 75, lz_mm = 28,
    vessels = {
      { id = "bath",    name = "Sous-Vide Bath",    kind = "pot",     shape = "tapered", lx_mm = 125, ly_mm = 75, lz_mm = 28 },
      { id = "skillet", name = "Cast-Iron Skillet", kind = "skillet", shape = "tapered", lx_mm = 125, ly_mm = 75, lz_mm = 28 }
    },
    items = {
      { id = "chicken_breast", mass_g = 210, vessel = "bath",    prep = "vacuum-sealed breast" },
      { id = "sea_salt",       mass_g = 2.4, vessel = "bath",    prep = "in pouch" },
      { id = "fresh_thyme",    mass_g = 3,   vessel = "bath",    prep = "in pouch" },
      { id = "olive_oil",      mass_g = 10,  vessel = "skillet", prep = "searing oil" }
    },
    steps = {
      { vessel = "bath",    action = "cook",    technique = "sous_vide", temp_c = 62.5, duration_sec = 3000,
        instruction = "Cook vacuum-sealed chicken breast in 62.5°C water bath for 50 min until thick head hits >=7.0D Salmonella lethality.",
        why = "Time-at-62.5°C achieves full pasteurization below 66°C actin syneresis, retaining ~9% more juice than 74°C roast." },
      { vessel = "bath",    action = "combine", target_vessel = "skillet", temp_c = 62, duration_sec = 10,
        instruction = "Unbag chicken, pat exterior bone-dry with paper towels, and lay into 210°C oiled Cast-Iron Skillet.",
        why = "Removing pouch moisture lets surface jump past 100°C immediately." },
      { vessel = "skillet", action = "cook",    technique = "pan_sear",  temp_c = 210,  duration_sec = 60, flip_midway = true,
        instruction = "Sear 30s per side at 210°C for golden Maillard crust, then slice immediately (no rest needed after isothermal cook).",
        why = "60s flash sear browns only the outer 1.5mm without overheating the 62.5°C interior." }
    }
  },
  {
    id = "citrus_ceviche", name = "Lime-Cured Sea Cod Ceviche", category = "seafood",
    notes = "Prep Board dicing (16mm) + Leche de Tigre bowl curing where citric acid denatures surface myosin cold.",
    shape = "cube", lx_mm = 18, ly_mm = 18, lz_mm = 18,
    vessels = {
      { id = "board", name = "Chilled Prep Board", kind = "board", shape = "slab",   lx_mm = 90, ly_mm = 60, lz_mm = 24, init_temp_c = 6 },
      { id = "bowl",  name = "Glass Curing Bowl",  kind = "bowl",  shape = "cube",   lx_mm = 16, ly_mm = 16, lz_mm = 16, init_temp_c = 6 }
    },
    items = {
      { id = "cod_fillet", mass_g = 180, vessel = "board", prep = "sushi-grade chilled loin" },
      { id = "lime_juice", mass_g = 55,  vessel = "bowl",  prep = "fresh key lime (pH 2.3)" },
      { id = "shallot",    mass_g = 20,  vessel = "bowl",  prep = "1mm feathered slices" },
      { id = "jalapeno",   mass_g = 8,   vessel = "bowl",  prep = "seeded brunoise" },
      { id = "sea_salt",   mass_g = 2.2, vessel = "bowl",  prep = "dissolved in lime" }
    },
    steps = {
      { vessel = "board", action = "cut",     cut_lx_mm = 16, cut_ly_mm = 16, cut_lz_mm = 16, cut_shape = "cube", temp_c = 6, duration_sec = 60,
        instruction = "Dice chilled 180g cod into clean 16mm cubes on Chilled Prep Board.",
        why = "16mm surface-to-volume ratio gives opaque acid-denatured exterior around a tender translucent center." },
      { vessel = "board", action = "combine", target_vessel = "bowl", temp_c = 6, duration_sec = 10, agitation = 0.30,
        instruction = "Fold cod cubes gently into Glass Curing Bowl with salted lime juice, shallot & jalapeno.",
        why = "Salt + citric acid drop aqueous pH <= 4.15." },
      { vessel = "bowl",  action = "cook",    technique = "cure_marinade", temp_c = 8, duration_sec = 1200,
        instruction = "Cure refrigerated at 8°C for 20 min, stirring once, and serve chilled.",
        why = "H+ protonation unfolds surface myosin into an opaque gel without a single joule of heat." }
    }
  },
  {
    id = "crispy_fried_tofu", name = "3D Corner-Crisp Fried Tofu with Ginger-Soy Glaze", category = "plant_protein",
    notes = "Cubed & starch-dusted tofu deep-fried in Wok 1, then tossed with reduced ginger-soy aromatics from Pan 2.",
    shape = "cube", lx_mm = 26, ly_mm = 26, lz_mm = 26,
    vessels = {
      { id = "wok",   name = "Deep-Fry Wok",     kind = "pot",     shape = "cube",   lx_mm = 26, ly_mm = 26, lz_mm = 26 },
      { id = "glaze", name = "Glaze Skillet",    kind = "skillet", shape = "liquid", lx_mm = 70, ly_mm = 70, lz_mm = 18 }
    },
    items = {
      { id = "firm_tofu",    mass_g = 200, vessel = "wok",   prep = "pressed 26mm cubes" },
      { id = "cornstarch",   mass_g = 18,  vessel = "wok",   prep = "dry crust dusting" },
      { id = "canola_oil",   mass_g = 25,  vessel = "wok",   prep = "frying oil uptake" },
      { id = "soy_sauce",    mass_g = 14,  vessel = "glaze", prep = "naturally brewed soy" },
      { id = "fresh_ginger", mass_g = 6,   vessel = "glaze", prep = "grated ginger" }
    },
    steps = {
      { vessel = "wok",   action = "cook",    technique = "deep_fry",       temp_c = 188, duration_sec = 240, flip_midway = true,
        instruction = "Press & dust 26mm tofu cubes in 18g cornstarch; deep-fry at 188°C for 4 min, tossing halfway.",
        why = "Amylose starch shell dehydrates below aw 0.35 while 3-face corners blister crisp." },
      { vessel = "glaze", action = "cook",    technique = "deglaze_reduce", temp_c = 98,  duration_sec = 60,  parallel = true,
        instruction = "Bubble 14g soy sauce + 6g grated ginger in Glaze Skillet for 60s until fragrant.",
        why = "Releases zingerone aroma and concentrates soy glutamate." },
      { vessel = "glaze", action = "combine", target_vessel = "wok",        temp_c = 95,  duration_sec = 10,  agitation = 0.55,
        instruction = "Drain oil and toss crisp tofu cubes briefly with warm ginger-soy glaze; serve immediately.",
        why = "Brief flash-toss coats micro-blisters with umami before moisture migrates inward." }
    }
  }
}

for _, r in ipairs(recipes) do
  fatduck.add_recipe(r)
end
