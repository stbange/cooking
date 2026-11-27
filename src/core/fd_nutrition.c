#include "fd_nutrition.h"

#include <math.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void fd_nutrition_update(fd_sim_state_t *sim) {
    if (!sim || !sim->initialized) return;
    fd_metrics_t *m = &sim->metrics;

    float fat_g  = fmaxf(0.0f, sim->total_fat_g + sim->absorbed_oil_g);
    float prot_g = sim->dry_protein_g;
    /* Retrograded starch RS3 + ungelatinized raw starch resist upper-gut amylase */
    float gel_frac   = clampf(m->starch_gelatinized - m->starch_retrograded, 0.0f, 1.0f);
    float rs_g       = sim->dry_starch_g * clampf(m->starch_retrograded + 0.65f * (1.0f - m->starch_gelatinized), 0.0f, 0.90f);
    float dig_starch = fmaxf(0.0f, sim->dry_starch_g - rs_g);
    float net_carbs  = dig_starch + sim->dry_sugar_g;

    float cooked_w = fmaxf(1.0f, prot_g + fat_g + sim->dry_starch_g + sim->dry_sugar_g +
                                 sim->dry_fiber_g + sim->dry_ash_g + sim->total_water_g);

    /* Atwater energy factors (resistant starch & soluble fiber ~2 kcal/g colonic SCFA) */
    float kcal = 4.0f * prot_g + 9.0f * fat_g + 4.0f * net_carbs + 2.0f * (sim->dry_fiber_g + rs_g);

    m->cooked_weight_g    = cooked_w;
    m->kcal_total         = kcal;
    m->kcal_per_100g      = kcal * (100.0f / cooked_w);
    m->protein_g          = prot_g;
    m->fat_g              = fat_g;
    m->net_carbs_g        = net_carbs;
    m->fiber_g            = sim->dry_fiber_g;
    m->resistant_starch_g = rs_g;
    m->sodium_mg          = sim->salt_g * 393.4f;

    /* Estimated Glycemic Index (eGI) & Glycemic Load (eGL) */
    float tot_avail_carb = sim->dry_starch_g + sim->dry_sugar_g;
    if (tot_avail_carb < 1.0f) {
        m->estimated_gi = 0.0f;
        m->estimated_gl = 0.0f;
    } else {
        float f_sug = sim->dry_sugar_g / tot_avail_carb;
        float f_sta = sim->dry_starch_g / tot_avail_carb;
        /* Gelatinized starch ~82 GI; native/retrograded starch ~26 GI; mixed culinary sugars ~62 GI */
        float base_gi = f_sug * 62.0f + f_sta * (gel_frac * 84.0f + (1.0f - gel_frac) * 26.0f);

        /* Gastric emptying attenuation from co-ingested fat, protein, fiber, and organic acids */
        float fat_ratio  = clampf(fat_g / (tot_avail_carb + 10.0f), 0.0f, 1.5f);
        float prot_ratio = clampf(prot_g / (tot_avail_carb + 10.0f), 0.0f, 1.5f);
        float fib_ratio  = clampf((sim->dry_fiber_g + rs_g) / (tot_avail_carb + 5.0f), 0.0f, 1.0f);
        float acid_atten = clampf((6.0f - m->ph) * 3.8f, 0.0f, 12.0f);

        float gi = base_gi * (1.0f - 0.14f * fat_ratio - 0.10f * prot_ratio - 0.18f * fib_ratio) - acid_atten;
        m->estimated_gi = clampf(gi, 10.0f, 98.0f);
        m->estimated_gl = (m->estimated_gi * net_carbs) / 100.0f;
    }

    /* Protein Digestibility / Bioavailability [0..1] */
    float unfold_gain = 0.30f * clampf(fmaxf(m->core_myosin_denat, m->gelatin_converted), 0.0f, 1.0f);
    float antinut_rem = sim->antinutrient_pool * (1.0f - clampf(m->core_myosin_denat, 0.0f, 1.0f));
    float char_loss   = 0.35f * m->max_char;
    m->protein_bioavailability = clampf(0.66f + unfold_gain - 0.32f * antinut_rem - char_loss, 0.25f, 0.98f);
}
