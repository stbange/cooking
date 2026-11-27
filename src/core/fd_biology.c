#include "fd_biology.h"
#include "fd_geometry.h"
#include "fd_physics.h"

#include <math.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

float fd_cardinal_temp_activity(float T_c, float t_min, float t_opt, float t_max) {
    if (T_c <= t_min || T_c >= t_max) return 0.0f;
    float num = (T_c - t_max) * (T_c - t_min) * (T_c - t_min);
    float den = (t_opt - t_min) * ((t_opt - t_min) * (T_c - t_opt) - (t_opt - t_max) * (t_opt + t_min - 2.0f * T_c));
    if (fabsf(den) < 1e-6f) return 0.0f;
    return clampf(num / den, 0.0f, 1.0f);
}

void fd_biology_step(fd_sim_state_t *sim, const fd_database_t *db, float dt_sec) {
    if (!sim || !sim->initialized || !db) return;
    fd_grid_t *g = &sim->grid;
    const fd_reactions_t *rx = &db->reactions;

    float d_ref = fmaxf(1.0f, sim->d_ref_sec);
    float t_ref = sim->t_ref_c;
    float z_val = fmaxf(1.0f, sim->z_val_c);

    float min_leth = 1e9f;
    float cold_t   = 1e9f;
    float max_t    = -1e9f;
    float max_char_score = -1.0f;
    int cx = 0, cy = 0, cz = 0;
    int hx = 0, hy = 0, hz = 0;

    float sum_w = 0.0f, sum_mail = 0.0f, sum_cara = 0.0f, sum_gel = 0.0f;
    float sum_sg = 0.0f, sum_sr = 0.0f, sum_por = 0.0f, sum_myo = 0.0f, sum_act = 0.0f;
    float surf_aw_sum = 0.0f, surf_mail_sum = 0.0f, max_char = 0.0f;
    int surf_cnt = 0, bio_cnt = 0;

    float w_inv = 1.0f / fmaxf(1.0f, sim->raw_weight_g);
    float x_sug = sim->dry_sugar_g * w_inv;

    for (int z = 0; z < g->nz; z++) {
        for (int y = 0; y < g->ny; y++) {
            for (int x = 0; x < g->nx; x++) {
                int i = fd_grid_idx(g, x, y, z);
                if (g->tissue[i] == FD_TISSUE_EMPTY) continue;
                float Tc = g->T[i];

                if (Tc > max_t) max_t = Tc;

                /* Bigelow microbial lethality integration (active above 48°C, plus FDA pH<=4.25 acid lethality) */
                if (g->tissue[i] != FD_TISSUE_BONE) {
                    if (dt_sec > 0.0f) {
                        if (Tc >= 48.0f) {
                            float exponent = clampf((Tc - t_ref) / z_val, -8.0f, 6.0f);
                            g->lethality_log10[i] = fminf(999.0f, g->lethality_log10[i] + (dt_sec / d_ref) * powf(10.0f, exponent));
                        }
                        if (sim->metrics.ph > 0.1f && sim->metrics.ph <= 4.25f) {
                            /* Citric/acetic acid microbial inactivation (ceviche & acidified mayo/aioli) */
                            float acid_rate = 0.048f * (4.35f - sim->metrics.ph);
                            g->lethality_log10[i] = fminf(999.0f, g->lethality_log10[i] + dt_sec * acid_rate);
                        }
                    }

                    if (g->lethality_log10[i] < min_leth ||
                        (fabsf(g->lethality_log10[i] - min_leth) < 1e-5f && Tc < cold_t)) {
                        min_leth = g->lethality_log10[i];
                        cold_t   = Tc;
                        cx = x; cy = y; cz = z;
                    }

                    /* Yeast fermentation CO2 porosity & oven-spring gas expansion (scaled by gluten network) */
                    float gluten_ret = 0.48f + 0.52f * sim->gluten_network;
                    if (dt_sec > 0.0f && sim->yeast_viability > 0.02f) {
                        float y_act = fd_cardinal_temp_activity(Tc, 4.0f, 32.0f, 49.0f) * sim->yeast_viability;
                        if (y_act > 0.0f && x_sug + sim->dry_starch_g * w_inv > 0.02f) {
                            g->porosity[i] = clampf(g->porosity[i] + 0.00022f * gluten_ret * y_act * dt_sec, 0.0f, 0.74f);
                        }
                        if (Tc > 54.0f) {
                            sim->yeast_viability *= expf(-0.08f * dt_sec);
                        }
                    }
                    /* Thermal leavening / steam oven-spring before gluten/albumen sets */
                    if (dt_sec > 0.0f && g->porosity[i] > 0.05f && Tc > 55.0f && Tc < 88.0f && g->denat_albumen[i] < 0.7f) {
                        g->porosity[i] = clampf(g->porosity[i] + 0.00095f * gluten_ret * dt_sec, 0.0f, 0.78f);
                    }

                    sum_w    += g->W[i];
                    sum_mail += g->maillard[i];
                    sum_cara += g->caramel[i];
                    sum_gel  += g->gelatin[i];
                    sum_sg   += g->starch_gel[i];
                    sum_sr   += g->starch_retro[i];
                    sum_por  += g->porosity[i];
                    sum_myo  += g->denat_myosin[i];
                    sum_act  += g->denat_actin[i];
                    if (g->char_level[i] > max_char) max_char = g->char_level[i];
                    bio_cnt++;
                }

                if (g->surface_mask[i] != 0 && g->tissue[i] != FD_TISSUE_BONE) {
                    surf_aw_sum   += fd_compute_water_activity(g->W[i], g->salt[i], x_sug);
                    surf_mail_sum += g->maillard[i];
                    surf_cnt++;
                    float c_rank = g->char_level[i] * 3.0f + g->maillard[i] + Tc * 0.002f;
                    if (c_rank > max_char_score) {
                        max_char_score = c_rank;
                        hx = x; hy = y; hz = z;
                    }
                }
            }
        }
    }

    /* Lactobacillus lactic acid fermentation & endogenous amylase starch->maltose conversion */
    if (dt_sec > 0.0f && bio_cnt > 0) {
        float avg_t = g->T[fd_grid_idx(g, g->nx / 2, g->ny / 2, g->nz / 2)];
        if (sim->lacto_viability > 0.02f) {
            float l_act = fd_cardinal_temp_activity(avg_t, 8.0f, 30.0f, 47.0f) * sim->lacto_viability;
            sim->acid_mol += 2.2e-6f * l_act * dt_sec * (sim->raw_weight_g * 0.001f);
            if (avg_t > 58.0f) sim->lacto_viability *= expf(-0.06f * dt_sec);
        }
        if (sim->amylase_viability > 0.02f && sim->dry_starch_g > 0.5f) {
            float a_act = fd_cardinal_temp_activity(avg_t, 35.0f, 65.0f, 79.0f) * sim->amylase_viability;
            float conv_g = fminf(sim->dry_starch_g * 0.25f, 0.00035f * a_act * dt_sec * sim->dry_starch_g);
            sim->dry_starch_g         -= conv_g;
            sim->dry_sugar_g          += conv_g;
            sim->dry_reducing_sugar_g += conv_g;
            if (avg_t > 77.0f) sim->amylase_viability *= expf(-0.12f * dt_sec);
        }
    }

    fd_metrics_t *m = &sim->metrics;
    float inv_bio  = 1.0f / (float)fmaxf(1, bio_cnt);
    float inv_surf = 1.0f / (float)fmaxf(1, surf_cnt);
    int center_idx = fd_grid_idx(g, g->nx / 2, g->ny / 2, g->nz / 2);

    m->surface_max_temp_c       = (max_t > -900.0f) ? max_t : 20.0f;
    m->core_temp_c              = g->T[center_idx];
    m->cold_spot_temp_c         = (cold_t < 900.0f) ? cold_t : m->core_temp_c;
    m->cold_spot_lethality_log10= (min_leth < 1e8f) ? min_leth : 0.0f;
    m->cold_x = cx; m->cold_y = cy; m->cold_z = cz;
    m->hot_x  = hx; m->hot_y  = hy; m->hot_z  = hz;

    m->avg_moisture        = sum_w * inv_bio;
    m->surface_aw          = (surf_cnt > 0) ? (surf_aw_sum * inv_surf) : 0.95f;
    m->crust_maillard      = fmaxf((surf_cnt > 0) ? (surf_mail_sum * inv_surf) : (sum_mail * inv_bio),
                                   0.45f * sim->fond_maillard);
    m->crust_caramel       = sum_cara * inv_bio;
    m->max_char            = max_char;
    m->core_myosin_denat   = g->denat_myosin[center_idx];
    m->core_actin_denat    = g->denat_actin[center_idx];
    m->gelatin_converted   = sum_gel * inv_bio;
    m->starch_gelatinized  = sum_sg * inv_bio;
    m->starch_retrograded  = sum_sr * inv_bio;
    m->avg_porosity        = sum_por * inv_bio;
    m->is_pasteurized_safe = (!sim->needs_pasteurization) ||
                             (m->cold_spot_lethality_log10 >= rx->target_pasteur_log10);

    /* Psychophysical 5-Taste, Aroma & 3D Texture Scoring */
    float cooked_w = fmaxf(5.0f, sim->dry_protein_g + sim->dry_starch_g + sim->dry_sugar_g +
                                 sim->dry_fiber_g + sim->dry_ash_g + sim->total_water_g +
                                 fmaxf(0.0f, sim->total_fat_g + sim->absorbed_oil_g));
    float inv_100g = 100.0f / cooked_w;

    float sugar_pct = sim->dry_sugar_g * inv_100g;
    float nacl_pct  = sim->salt_g * inv_100g;
    float glu_100   = sim->glutamate_mg * inv_100g + 85.0f * m->crust_maillard;
    float nuc_100   = sim->nucleotides_mg * inv_100g;
    float umami_eff = glu_100 * (1.0f + rx->umami_synergy_gamma * nuc_100);

    m->sweet    = clampf(10.0f * (sugar_pct / (sugar_pct + 7.5f)) + 1.8f * m->crust_caramel, 0.0f, 10.0f);
    m->salty    = clampf(10.0f * (nacl_pct / (nacl_pct + 1.15f)), 0.0f, 10.0f);
    m->sour     = clampf(2.2f * fmaxf(0.0f, 6.2f - m->ph), 0.0f, 10.0f);
    /* Salt cross-modal suppression of bitterness; pyrolysis char & smoke add strong bitterness */
    float raw_bitter = sim->bitter_pool * 4.0f + 9.5f * m->max_char + (m->oil_smoked ? 3.5f : 0.0f);
    m->bitter   = clampf(raw_bitter * (1.0f - 0.05f * m->salty), 0.0f, 10.0f);
    m->umami    = clampf(10.0f * log1pf(umami_eff * 0.025f) / log1pf(25.0f), 0.0f, 10.0f);
    m->pungency = clampf(sim->pungency_pool * (1.0f + 0.30f * sim->cell_rupture) * 10.0f, 0.0f, 10.0f);
    float aroma_eff = sim->aroma_pool * (1.0f + 0.32f * sim->cell_rupture);
    m->aroma    = clampf(aroma_eff * 4.5f + 4.8f * m->crust_maillard + 3.2f * m->crust_caramel, 0.0f, 10.0f);

    /* 3D Texture Mechanics */
    m->crispness = clampf(10.0f * (1.0f - m->surface_aw / 0.78f) *
                          (0.55f + 0.45f * fmaxf(m->crust_maillard, m->starch_gelatinized)), 0.0f, 10.0f);
    float raw_collagen_pen = (sim->dry_collagen_g * w_inv > 0.02f) ? (4.0f * (1.0f - m->gelatin_converted)) : 0.0f;
    m->tenderness = clampf(6.8f + 2.5f * (sum_myo * inv_bio) + 2.8f * m->gelatin_converted
                           + 2.2f * m->avg_porosity + 1.6f * sim->mech_tender
                           - 4.2f * (sum_act * inv_bio) * (sum_act * inv_bio) - raw_collagen_pen, 0.0f, 10.0f);
    float fat_pct = fmaxf(0.0f, sim->total_fat_g + sim->absorbed_oil_g) * inv_100g;
    m->juiciness  = clampf(9.2f * m->avg_moisture + 0.14f * fat_pct + 2.2f * m->gelatin_converted
                           + 0.65f * sim->rested_frac, 0.0f, 10.0f);

    /* Composite Culinary Harmony Score [0..100] */
    float salt_harmony = (m->sweet > 4.5f) ? 7.5f : (10.0f - 2.2f * fabsf(nacl_pct - 1.05f));
    float starch_pen   = (sim->dry_starch_g * w_inv > 0.08f && m->starch_gelatinized < 0.72f)
                         ? (28.0f * (0.72f - m->starch_gelatinized)) : 0.0f;
    float emul_pen     = (m->emulsion_stability < 0.55f) ? (30.0f * (0.55f - m->emulsion_stability)) : 0.0f;
    float char_pen     = (m->max_char > 0.12f) ? (55.0f * (m->max_char - 0.12f)) : 0.0f;
    float safe_pen     = m->is_pasteurized_safe ? 0.0f : 40.0f;

    float base_score = 22.0f + 1.9f * clampf(salt_harmony, 0.0f, 10.0f)
                     + 1.8f * m->aroma + 1.5f * fmaxf(m->umami, m->sweet)
                     + 1.6f * m->tenderness + 1.4f * m->juiciness + 1.1f * m->crispness
                     + 0.7f * fminf(m->sour, 4.5f)
                     - 2.2f * fmaxf(0.0f, m->bitter - 2.0f)
                     - starch_pen - emul_pen - char_pen - safe_pen;
    m->overall_score = clampf(base_score, 0.0f, 100.0f);
}
