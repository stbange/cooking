#include "fd_chemistry.h"
#include "fd_physics.h"

#include <math.h>

#define FD_GAS_R 8.314462618

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

double fd_arrhenius_rate(fd_arrhenius_t arr, float temp_c) {
    double tk = (double)clampf(temp_c, -40.0f, 380.0f) + 273.15;
    double exponent = -arr.ea / (FD_GAS_R * tk);
    if (exponent < -85.0) return 0.0;
    if (exponent > 20.0)  exponent = 20.0;
    return arr.a * exp(exponent);
}

float fd_compute_ph(float acid_mol, float base_mol, float raw_weight_g, float base_ph) {
    float mass_kg = fmaxf(0.01f, raw_weight_g * 0.001f);
    float net_acid_m = (acid_mol - base_mol) / mass_kg;
    float shift = (net_acid_m >= 0.0f)
                ? (-1.65f * log1pf(48.0f * net_acid_m))
                : ( 1.45f * log1pf(36.0f * (-net_acid_m)));
    return clampf(base_ph + shift, 2.2f, 9.2f);
}

static float step_first_order(float cur, double k, float dt) {
    if (k <= 0.0 || dt <= 0.0f) return cur;
    double rem = (1.0 - (double)cur) * exp(-k * (double)dt);
    return clampf((float)(1.0 - rem), 0.0f, 1.0f);
}

void fd_chemistry_step(fd_sim_state_t *sim, const fd_database_t *db, float dt_sec) {
    if (!sim || !sim->initialized || !db || dt_sec <= 0.0f) return;
    fd_grid_t *g = &sim->grid;
    const fd_reactions_t *rx = &db->reactions;

    float w_inv = 1.0f / fmaxf(1.0f, sim->raw_weight_g);
    float x_sug = sim->dry_sugar_g * w_inv;
    float x_red = sim->dry_reducing_sugar_g * w_inv;
    float x_sta = sim->dry_starch_g * w_inv;
    float x_col = sim->dry_collagen_g * w_inv;

    /* Mixture pH equilibrium in the aqueous + protein buffer phase (oil is non-aqueous) */
    float base_ph = (sim->primary_ing_idx >= 0 && sim->primary_ing_idx < db->ingredient_count)
                    ? db->ingredients[sim->primary_ing_idx].ph : 6.1f;
    float aq_mass_g = fmaxf(8.0f, sim->total_water_g + 0.35f * sim->dry_protein_g);
    float ph = fd_compute_ph(sim->acid_mol, sim->base_mol, aq_mass_g, base_ph);
    sim->metrics.ph = ph;

    /* Alkaline pH accelerates Maillard; extreme acid (<4.3) denatures muscle proteins (ceviche!) */
    float ph_maillard = clampf(0.35f + 0.30f * (ph - 4.5f), 0.15f, 1.85f);
    float acid_denat_k = (ph < 4.35f) ? (0.0018f * (4.35f - ph)) : 0.0f;

    /* Precursor availability factor for Maillard (amino + reducing sugars/carbs) */
    float maillard_prec = clampf(0.18f + 2.8f * sim->free_amino_pool * (x_red + 0.12f * x_sta + 0.03f), 0.08f, 1.6f);
    float caramel_prec  = clampf(0.12f + 4.5f * x_sug, 0.05f, 1.8f);

    float max_surf_t = -100.0f;
    for (int i = 0; i < g->count; i++) {
        if (g->tissue[i] == FD_TISSUE_EMPTY || g->tissue[i] == FD_TISSUE_BONE) continue;
        float Tc = g->T[i];
        if (Tc > max_surf_t) max_surf_t = Tc;

        float aw = fd_compute_water_activity(g->W[i], g->salt[i], x_sug);

        /* Bell-shaped water activity factor for Maillard peaking around aw ~ 0.65 */
        float daw = (aw - 0.65f) / 0.25f;
        float aw_maillard = clampf(expf(-0.5f * daw * daw), 0.05f, 1.0f);

        double k_mail = fd_arrhenius_rate(rx->maillard, Tc) * (double)(maillard_prec * aw_maillard * ph_maillard);
        double k_cara = fd_arrhenius_rate(rx->caramel, Tc)  * (double)(caramel_prec * clampf(1.25f - aw, 0.15f, 1.2f));
        double k_char = (Tc > 175.0f) ? fd_arrhenius_rate(rx->pyrolysis, Tc) * (double)clampf(1.2f - aw, 0.2f, 1.2f) : 0.0;

        g->maillard[i]   = step_first_order(g->maillard[i],   k_mail, dt_sec);
        g->caramel[i]    = step_first_order(g->caramel[i],    k_cara, dt_sec);
        g->char_level[i] = step_first_order(g->char_level[i], k_char, dt_sec);

        /* Multi-stage protein denaturation + actin moisture syneresis */
        double k_myo = fd_arrhenius_rate(rx->myosin, Tc) + (double)acid_denat_k;
        double k_act = fd_arrhenius_rate(rx->actin, Tc)  + (double)(acid_denat_k * 0.25f);
        /* Egg yolk sets around ~65°C whereas ovalbumin/gluten sets around ~80°C */
        float alb_shift_t = (g->tissue[i] == FD_TISSUE_YOLK_CORE) ? (Tc + 14.0f) : Tc;
        double k_alb = fd_arrhenius_rate(rx->albumen, alb_shift_t);

        float prev_actin = g->denat_actin[i];
        g->denat_myosin[i]  = step_first_order(g->denat_myosin[i],  k_myo, dt_sec);
        g->denat_actin[i]   = step_first_order(g->denat_actin[i],   k_act, dt_sec);
        g->denat_albumen[i] = step_first_order(g->denat_albumen[i], k_alb, dt_sec);

        float d_actin = g->denat_actin[i] - prev_actin;
        if (d_actin > 0.0f && g->W[i] > 0.42f) {
            /* Actin myofibril shrinkage expels bound water (syneresis) */
            float squeeze = 0.16f * d_actin * (g->W[i] - 0.42f);
            g->W[i] -= squeeze;
            sim->total_water_g = fmaxf(1.0f, sim->total_water_g - squeeze * sim->raw_weight_g / (float)fmaxf(1, g->active_count));
        }

        /* Hydrated collagen -> gelatin hydrolysis */
        if (x_col > 0.002f && g->W[i] > 0.24f) {
            double k_col = fd_arrhenius_rate(rx->collagen, Tc) * (double)clampf(g->W[i] / 0.55f, 0.2f, 1.1f);
            g->gelatin[i] = step_first_order(g->gelatin[i], k_col, dt_sec);
        }

        /* Hydrated starch gelatinization vs. cold-chill retrogradation (RS3 resistant starch) */
        if (x_sta > 0.01f) {
            if (Tc > 52.0f && g->W[i] > 0.22f) {
                double k_sg = fd_arrhenius_rate(rx->starch_gel, Tc) * (double)clampf((g->W[i] - 0.18f) / 0.40f, 0.1f, 1.2f);
                g->starch_gel[i] = step_first_order(g->starch_gel[i], k_sg, dt_sec);
            } else if (Tc >= -1.0f && Tc <= 14.0f && g->starch_gel[i] > 0.05f) {
                /* Peak retrogradation nucleation around 4°C */
                float cold_bell = expf(-0.08f * (Tc - 4.0f) * (Tc - 4.0f));
                float retro_step = rx->retrogradation_rate * cold_bell * dt_sec * g->starch_gel[i];
                retro_step = fminf(retro_step, g->starch_gel[i] * 0.45f);
                g->starch_retro[i] = clampf(g->starch_retro[i] + retro_step, 0.0f, 0.55f);
            }
        }
    }

    /* Salt diffusion from surface brine/seasoning into outer cells */
    if (sim->salt_g > 0.01f) {
        float target_salt = clampf(sim->salt_g * w_inv, 0.0f, 0.12f);
        for (int i = 0; i < g->count; i++) {
            if (g->tissue[i] == FD_TISSUE_EMPTY) continue;
            float rate = (g->surface_mask[i] != 0) ? 0.004f : 0.0006f;
            g->salt[i] += (target_salt - g->salt[i]) * fminf(1.0f, rate * dt_sec);
        }
    }

    if (sim->smoke_point_min_c > 50.0f && max_surf_t > sim->smoke_point_min_c) {
        sim->metrics.oil_smoked = true;
    }
}
