#ifndef FD_TYPES_H
#define FD_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FD_MAX_GRID_DIM     32
#define FD_MAX_CELLS        (FD_MAX_GRID_DIM * FD_MAX_GRID_DIM * FD_MAX_GRID_DIM)
#define FD_MAX_INGREDIENTS  160
#define FD_MAX_TECHNIQUES   24
#define FD_MAX_RECIPES      64
#define FD_MAX_STEPS        8
#define FD_MAX_RECIPE_ITEMS 10
#define FD_HISTORY_LEN      180

/* Voxel tissue occupancy types */
typedef enum {
    FD_TISSUE_EMPTY      = 0,
    FD_TISSUE_LEAN       = 1,
    FD_TISSUE_FAT_CAP    = 2,
    FD_TISSUE_SKIN       = 3,
    FD_TISSUE_BONE       = 4,
    FD_TISSUE_YOLK_CORE  = 5
} fd_tissue_t;

/* 3D exposed face bitmask on each active voxel */
enum {
    FD_FACE_NEG_X = 1 << 0,
    FD_FACE_POS_X = 1 << 1,
    FD_FACE_NEG_Y = 1 << 2,
    FD_FACE_POS_Y = 1 << 3,
    FD_FACE_NEG_Z = 1 << 4, /* Bottom pan contact face */
    FD_FACE_POS_Z = 1 << 5  /* Top open / radiant face */
};

/* 3D geometry archetypes */
typedef enum {
    FD_SHAPE_LIQUID         = 0,
    FD_SHAPE_SLAB           = 1,
    FD_SHAPE_CUBE           = 2,
    FD_SHAPE_CYLINDER       = 3,
    FD_SHAPE_SPHERE         = 4,
    FD_SHAPE_TAPERED_FILLET = 5,
    FD_SHAPE_FAT_CAP_SLAB   = 6,
    FD_SHAPE_BONE_IN_SLAB   = 7,
    FD_SHAPE_CONCENTRIC_EGG = 8
} fd_shape_type_t;

/* Dietary and functional role bitmasks */
enum {
    FD_DIET_VEGAN       = 1u << 0,
    FD_DIET_VEGETARIAN  = 1u << 1,
    FD_DIET_GLUTEN_FREE = 1u << 2,
    FD_DIET_DAIRY_FREE  = 1u << 3
};

enum {
    FD_ROLE_PROTEIN_MAIN = 1u << 0,
    FD_ROLE_VEG_MAIN     = 1u << 1,
    FD_ROLE_STARCH_BASE  = 1u << 2,
    FD_ROLE_FAT_MEDIUM   = 1u << 3,
    FD_ROLE_LIQUID_BASE  = 1u << 4,
    FD_ROLE_ACID         = 1u << 5,
    FD_ROLE_UMAMI        = 1u << 6,
    FD_ROLE_SWEETENER    = 1u << 7,
    FD_ROLE_EMULSIFIER   = 1u << 8,
    FD_ROLE_AROMATIC     = 1u << 9,
    FD_ROLE_SALT         = 1u << 10,
    FD_ROLE_LEAVENER     = 1u << 11
};

typedef struct {
    char id[48];
    char name[64];
    char category[32];

    /* Raw mass fractions summing to ~1.0 */
    float water;
    float protein;
    float fat;
    float starch;
    float sugar;
    float fiber;
    float ash;

    /* Sub-constituents & chemical properties */
    float collagen_frac;       /* Fraction of protein that is connective collagen */
    float reducing_sugar_frac; /* Fraction of sugar participating strongly in Maillard */
    float free_amino;          /* Relative free amine reactivity [0..1] */
    float ph;                  /* Natural pH [2..9] */
    float acidity_molar;       /* Titratable acid equivalents mol/kg */
    float sodium_mg_100g;
    float glutamate_mg_100g;
    float nucleotides_mg_100g; /* 5'-IMP + GMP for synergistic umami */
    float emulsifier_power;    /* Lecithin / casein / mucilage capacity [0..1] */
    float smoke_point_c;       /* Thermal lipid breakdown onset (°C) */
    float pungency;            /* Trigeminal heat [0..1] */
    float aroma_potency;       /* Base volatile aromatics [0..1] */
    float bitter_base;         /* Intrinsic alkaloid/tannin bitterness [0..1] */

    /* Biological & enzymatic parameters */
    float amylase_activity;
    float yeast_activity;
    float lacto_activity;
    float antinutrient_frac;   /* Avidin / lectins / trypsin inhibitors [0..1] */
    bool  requires_pasteurization;
    float d_ref_sec;           /* Bigelow decimal reduction time at t_ref_c */
    float t_ref_c;
    float z_val_c;

    /* 3D geometric defaults */
    float fiber_anisotropy;    /* >= 1.0 thermal/moisture boost along X grain */
    fd_shape_type_t default_shape;
    float default_lx_mm;
    float default_ly_mm;
    float default_lz_mm;

    uint32_t diet_flags;
    uint32_t role_flags;
} fd_ingredient_t;

typedef struct {
    char id[32];
    char name[48];
    float h_bottom;          /* W/(m^2*K) on -Z pan/grate face */
    float h_top;             /* W/(m^2*K) on +Z top face */
    float h_side;            /* W/(m^2*K) on +-X, +-Y faces */
    float emissivity_rad;    /* Radiant view*emissivity [0..1] */
    float ambient_humidity;  /* Relative humidity [0..1] */
    float default_temp_c;
    float min_temp_c;
    float max_temp_c;
    float oil_uptake_rate;   /* Crust capillary oil absorption factor */
    float shear_rate;        /* Mechanical agitation / whisking s^-1 */
    bool  immersed_water;
    bool  immersed_oil;
    bool  sealed_pouch;      /* Sous-vide zero evaporative mass loss */
} fd_technique_t;

typedef struct {
    double a;
    double ea; /* J/mol */
} fd_arrhenius_t;

typedef struct {
    fd_arrhenius_t maillard;
    fd_arrhenius_t caramel;
    fd_arrhenius_t pyrolysis;
    fd_arrhenius_t myosin;
    fd_arrhenius_t actin;
    fd_arrhenius_t albumen;
    fd_arrhenius_t collagen;
    fd_arrhenius_t starch_gel;
    float retrogradation_rate;
    float target_pasteur_log10;
    float umami_synergy_gamma;
} fd_reactions_t;

typedef struct {
    char ingredient_id[48];
    float mass_g;
} fd_recipe_item_t;

typedef struct {
    char technique_id[32];
    float temp_c;
    float duration_sec;
    bool  flip_midway;
    bool  lid_closed;
} fd_recipe_step_t;

typedef struct {
    char id[48];
    char name[72];
    char category[32];
    char notes[160];
    fd_shape_type_t shape;
    float lx_mm, ly_mm, lz_mm;
    int item_count;
    fd_recipe_item_t items[FD_MAX_RECIPE_ITEMS];
    int step_count;
    fd_recipe_step_t steps[FD_MAX_STEPS];
    bool chill_after; /* Triggers cold-chill starch retrogradation benchmark */
} fd_recipe_t;

/* 3D Volumetric Voxel Grid (dynamically sized to nx*ny*nz) */
typedef struct {
    int nx, ny, nz;
    int count;
    int active_count;
    float lx, ly, lz; /* Physical size in meters */
    float dx, dy, dz;
    float fiber_anisotropy;

    uint8_t *tissue;
    uint8_t *surface_mask;

    float *T;
    float *W;
    float *ice;
    float *fat;
    float *maillard;
    float *caramel;
    float *char_level;
    float *denat_myosin;
    float *denat_actin;
    float *denat_albumen;
    float *gelatin;
    float *starch_gel;
    float *starch_retro;
    float *porosity;
    float *salt;
    float *lethality_log10;

    /* Per-step Choi-Okos thermal scratch buffers (heap-allocated for 64KB WASM stack safety) */
    float *rho_arr;
    float *rho_cp;
    float *k_cond;
} fd_grid_t;

/* Evaluated Dish Metrics (Physics, Chemistry, Biology, Sensory, Nutrition) */
typedef struct {
    float surface_max_temp_c;
    float core_temp_c;
    float cold_spot_temp_c;
    float cold_spot_lethality_log10;
    int   cold_x, cold_y, cold_z;
    int   hot_x, hot_y, hot_z;

    float avg_moisture;
    float surface_aw;
    float crust_maillard;
    float crust_caramel;
    float max_char;
    float core_myosin_denat;
    float core_actin_denat;
    float gelatin_converted;
    float starch_gelatinized;
    float starch_retrograded;
    float avg_porosity;

    float ph;
    float emulsion_stability;  /* [0..1], <0.4 means split/broken */
    float emulsion_viscosity_cp;
    bool  oil_smoked;
    bool  is_pasteurized_safe;

    /* 5 primary tastes + aroma + texture [0..10] */
    float sweet;
    float salty;
    float sour;
    float bitter;
    float umami;
    float aroma;
    float pungency;
    float crispness;
    float tenderness;
    float juiciness;
    float overall_score;       /* Composite culinary harmony [0..100] */

    /* Cooked nutrition & bioavailability */
    float cooked_weight_g;
    float kcal_total;
    float kcal_per_100g;
    float protein_g;
    float fat_g;
    float net_carbs_g;
    float fiber_g;
    float resistant_starch_g;
    float sodium_mg;
    float estimated_gi;        /* [0..100] */
    float estimated_gl;
    float protein_bioavailability; /* [0..1] */
} fd_metrics_t;

/* Full Coupled Cooking Station State */
typedef struct {
    fd_grid_t grid;
    bool initialized;

    /* Coupled bulk composition & surrounding cooking medium */
    int item_count;
    int ing_indices[FD_MAX_RECIPE_ITEMS];
    float ing_masses_g[FD_MAX_RECIPE_ITEMS];

    int primary_ing_idx;
    int active_tech_idx;
    float ambient_temp_c;
    bool  lid_closed;
    float shear_boost;
    double elapsed_sec;

    /* Bulk solid/liquid chemical pool */
    float raw_weight_g;
    float dry_protein_g;
    float dry_collagen_g;
    float dry_starch_g;
    float dry_sugar_g;
    float dry_reducing_sugar_g;
    float dry_fiber_g;
    float dry_ash_g;
    float total_water_g;
    float total_fat_g;
    float absorbed_oil_g;
    float salt_g;
    float acid_mol;
    float base_mol;
    float glutamate_mg;
    float nucleotides_mg;
    float emulsifier_units;
    float free_amino_pool;
    float aroma_pool;
    float bitter_pool;
    float pungency_pool;
    float smoke_point_min_c;
    float antinutrient_pool;

    /* Biological living states [0..1] */
    float yeast_viability;
    float lacto_viability;
    float amylase_viability;
    bool  needs_pasteurization;
    float d_ref_sec, t_ref_c, z_val_c;

    /* Telemetry ring buffer for real-time workbench plots */
    int   hist_count;
    int   hist_head;
    float hist_time[FD_HISTORY_LEN];
    float hist_t_surf[FD_HISTORY_LEN];
    float hist_t_cold[FD_HISTORY_LEN];
    float hist_maillard[FD_HISTORY_LEN];
    float hist_moisture[FD_HISTORY_LEN];
    double last_hist_sec;

    /* Audio physical flux outputs */
    float sizzle_flux;
    float boil_flux;

    fd_metrics_t metrics;
} fd_sim_state_t;

/* Culinary Database Registry populated from Lua */
typedef struct {
    int ingredient_count;
    fd_ingredient_t ingredients[FD_MAX_INGREDIENTS];

    int technique_count;
    fd_technique_t techniques[FD_MAX_TECHNIQUES];

    int recipe_count;
    fd_recipe_t recipes[FD_MAX_RECIPES];

    fd_reactions_t reactions;
} fd_database_t;

/* Solver Constraints for Proposing & Inventing Recipes */
typedef struct {
    int   include_count;
    char  include_ids[6][48];
    int   exclude_count;
    char  exclude_ids[6][48];

    uint32_t required_diet_flags;
    float max_time_sec;
    float max_kcal_per_100g;
    float max_gi;
    float min_protein_g;

    /* Target sensory profile [0..10], negative = unconstrained */
    float target_crispness;
    float target_tenderness;
    float target_umami;
    float target_sweet;
    float target_sour;
    bool  require_emulsion;
    uint32_t rng_seed;
} fd_constraints_t;

#endif /* FD_TYPES_H */
