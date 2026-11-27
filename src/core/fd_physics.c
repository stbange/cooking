#include "fd_physics.h"
#include "fd_geometry.h"

#include <math.h>
#include <string.h>

#define FD_LATENT_FUSION_J_KG 333600.0f
#define FD_LATENT_VAP_J_KG    2257000.0f
#define FD_SIGMA_SB           5.6703744e-8f

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void fd_choi_okos(float T_c, float x_water, float x_ice, float x_prot,
                  float x_fat, float x_carb, float x_fiber, float x_ash, float porosity,
                  float *out_rho, float *out_cp_eff, float *out_k) {
    float Tc = clampf(T_c, -40.0f, 260.0f);
    float sum = x_water + x_ice + x_prot + x_fat + x_carb + x_fiber + x_ash;
    if (sum < 1e-4f) { x_water = 1.0f; sum = 1.0f; }
    float inv = 1.0f / sum;
    x_water *= inv; x_ice *= inv; x_prot *= inv; x_fat *= inv;
    x_carb  *= inv; x_fiber *= inv; x_ash *= inv;

    /* Choi & Okos (1986) constituent specific heats J/(kg*K) */
    float cp_w = 4176.2f + 0.0909f * Tc;
    float cp_i = 2062.3f + 6.0769f * Tc;
    float cp_p = 2008.2f + 1.2089f * Tc;
    float cp_f = 1984.2f + 1.4733f * Tc;
    float cp_c = 1548.8f + 1.9625f * Tc;
    float cp_b = 1845.9f + 1.8306f * Tc;
    float cp_a = 1092.6f + 1.8896f * Tc;
    float cp = x_water * cp_w + x_ice * cp_i + x_prot * cp_p + x_fat * cp_f +
               x_carb * cp_c + x_fiber * cp_b + x_ash * cp_a;

    /* Apparent enthalpy peak across ice-water fusion (-2.5°C .. +0.5°C) */
    float w_tot = x_water + x_ice;
    if (Tc > -6.0f && Tc < 3.0f && w_tot > 0.01f) {
        float d = (Tc + 1.2f) / 1.1f;
        cp += w_tot * FD_LATENT_FUSION_J_KG * 0.51f * expf(-0.5f * d * d);
    }

    /* Choi & Okos constituent thermal conductivities W/(m*K) */
    float k_w = 0.5711f + 1.762e-3f * Tc - 6.703e-6f * Tc * Tc;
    float k_i = 2.2196f - 6.248e-3f * Tc;
    float k_p = 0.1788f + 1.195e-3f * Tc;
    float k_f = 0.1807f - 2.760e-4f * Tc;
    float k_c = 0.2014f + 1.387e-3f * Tc;
    float k_b = 0.1833f + 1.249e-3f * Tc;
    float k_a = 0.3296f + 1.401e-3f * Tc;

    /* Constituent densities kg/m^3 */
    float r_w = 997.2f - 0.15f * Tc;
    float r_i = 916.9f - 0.13f * Tc;
    float r_p = 1329.9f - 0.52f * Tc;
    float r_f = 925.6f - 0.42f * Tc;
    float r_c = 1599.1f - 0.31f * Tc;
    float r_b = 1311.5f - 0.37f * Tc;
    float r_a = 2423.8f - 0.28f * Tc;

    float v_w = x_water / r_w, v_i = x_ice / r_i, v_p = x_prot / r_p, v_f = x_fat / r_f;
    float v_c = x_carb / r_c,  v_b = x_fiber / r_b, v_a = x_ash / r_a;
    float v_tot = v_w + v_i + v_p + v_f + v_c + v_b + v_a;
    float eps = clampf(porosity, 0.0f, 0.85f);
    float rho_solid = 1.0f / fmaxf(v_tot, 1e-5f);
    float k_solid = (v_w * k_w + v_i * k_i + v_p * k_p + v_f * k_f + v_c * k_c + v_b * k_b + v_a * k_a) / fmaxf(v_tot, 1e-5f);

    if (out_rho)    *out_rho    = fmaxf(120.0f, (1.0f - eps) * rho_solid + eps * 1.1f);
    if (out_cp_eff) *out_cp_eff = fmaxf(850.0f, cp);
    if (out_k)      *out_k      = fmaxf(0.045f, (1.0f - eps) * k_solid + eps * 0.032f);
}

float fd_compute_water_activity(float water_frac, float salt_frac, float sugar_frac) {
    if (water_frac <= 0.005f) return 0.05f;
    /* Raoult's mole-fraction depression (NaCl dissociates -> 2 ions, MW 58.44; sugar MW ~180) */
    float n_w   = water_frac / 0.018015f;
    float n_s   = 2.0f * (fmaxf(0.0f, salt_frac) / 0.05844f);
    float n_sug = fmaxf(0.0f, sugar_frac) / 0.18016f;
    float aw = n_w / (n_w + n_s + n_sug + 0.08f);
    /* Desiccation sorption isotherm roll-off at low moisture */
    if (water_frac < 0.22f) {
        aw *= clampf(water_frac / 0.22f, 0.05f, 1.0f);
    }
    return clampf(aw, 0.05f, 0.998f);
}

/* Linearized effective convective+radiative coefficient on an exposed voxel face */
static float face_h_eff(const fd_technique_t *tech, uint8_t face_bit, float T_surf, float T_env, bool lid_closed) {
    float h = tech->h_side;
    if (face_bit == FD_FACE_NEG_Z) h = tech->h_bottom;
    else if (face_bit == FD_FACE_POS_Z) h = lid_closed ? fmaxf(tech->h_top * 2.2f, 45.0f) : tech->h_top;

    if (tech->emissivity_rad > 0.0f && face_bit != FD_FACE_NEG_Z) {
        float ts_k = clampf(T_surf, -40.0f, 350.0f) + 273.15f;
        float te_k = clampf(T_env, -40.0f, 450.0f) + 273.15f;
        /* Linearized Stefan-Boltzmann: eps*sigma*(Te^2 + Ts^2)*(Te + Ts) */
        h += tech->emissivity_rad * FD_SIGMA_SB * (te_k * te_k + ts_k * ts_k) * (te_k + ts_k);
    }
    return h;
}

/* Solve 1D implicit tridiagonal system along a single active voxel segment */
static void solve_tridiag_segment(int len, const float *a, const float *b, const float *c, const float *d, float *x) {
    float cp[FD_MAX_GRID_DIM];
    float dp[FD_MAX_GRID_DIM];
    float denom = b[0];
    cp[0] = (len > 1) ? (c[0] / denom) : 0.0f;
    dp[0] = d[0] / denom;
    for (int i = 1; i < len; i++) {
        denom = b[i] - a[i] * cp[i - 1];
        if (fabsf(denom) < 1e-9f) denom = 1e-9f;
        cp[i] = (i < len - 1) ? (c[i] / denom) : 0.0f;
        dp[i] = (d[i] - a[i] * dp[i - 1]) / denom;
    }
    x[len - 1] = dp[len - 1];
    for (int i = len - 2; i >= 0; i--) {
        x[i] = dp[i] - cp[i] * x[i + 1];
    }
}

/* Sweep 1D implicit diffusion along axis (0=X, 1=Y, 2=Z) with Robin boundary conditions */
static void sweep_heat_axis(fd_grid_t *g, int axis, float dt, const float *rho_cp, const float *k_cond,
                            const fd_technique_t *tech, float T_env, bool lid_closed) {
    int n_axis = (axis == 0) ? g->nx : ((axis == 1) ? g->ny : g->nz);
    int n_u    = (axis == 0) ? g->ny : g->nx;
    int n_v    = (axis == 2) ? g->ny : g->nz;
    float ds   = (axis == 0) ? g->dx : ((axis == 1) ? g->dy : g->dz);
    float aniso = (axis == 0) ? g->fiber_anisotropy : 1.0f;
    uint8_t neg_bit = (axis == 0) ? FD_FACE_NEG_X : ((axis == 1) ? FD_FACE_NEG_Y : FD_FACE_NEG_Z);
    uint8_t pos_bit = (axis == 0) ? FD_FACE_POS_X : ((axis == 1) ? FD_FACE_POS_Y : FD_FACE_POS_Z);

    float a[FD_MAX_GRID_DIM], b[FD_MAX_GRID_DIM], c[FD_MAX_GRID_DIM], d[FD_MAX_GRID_DIM], sol[FD_MAX_GRID_DIM];
    int map_idx[FD_MAX_GRID_DIM];

    for (int v = 0; v < n_v; v++) {
        for (int u = 0; u < n_u; u++) {
            int i = 0;
            while (i < n_axis) {
                int x = (axis == 0) ? i : u;
                int y = (axis == 0) ? u : ((axis == 1) ? i : v);
                int z = (axis == 2) ? i : v;
                int idx0 = fd_grid_idx(g, x, y, z);
                if (g->tissue[idx0] == FD_TISSUE_EMPTY) { i++; continue; }

                int seg_len = 0;
                while (i < n_axis) {
                    x = (axis == 0) ? i : u;
                    y = (axis == 0) ? u : ((axis == 1) ? i : v);
                    z = (axis == 2) ? i : v;
                    int idx = fd_grid_idx(g, x, y, z);
                    if (g->tissue[idx] == FD_TISSUE_EMPTY) break;
                    map_idx[seg_len++] = idx;
                    i++;
                }

                for (int s = 0; s < seg_len; s++) {
                    int idx = map_idx[s];
                    float cap = rho_cp[idx] * ds / dt; /* W/(m^2*K) capacity scale */
                    float k_i = k_cond[idx] * aniso;
                    float cond_l = 0.0f, cond_r = 0.0f;

                    if (s > 0) {
                        float k_l = k_cond[map_idx[s - 1]] * aniso;
                        cond_l = (2.0f * k_l * k_i / fmaxf(k_l + k_i, 1e-5f)) / ds;
                    }
                    if (s < seg_len - 1) {
                        float k_r = k_cond[map_idx[s + 1]] * aniso;
                        cond_r = (2.0f * k_r * k_i / fmaxf(k_r + k_i, 1e-5f)) / ds;
                    }

                    a[s] = -cond_l;
                    c[s] = -cond_r;
                    b[s] = cap + cond_l + cond_r;
                    d[s] = cap * g->T[idx];

                    /* Apply Robin boundary flux on exposed segment ends (well-mixed 1x1x1 skips solid Biot lag) */
                    if (g->surface_mask[idx] & neg_bit) {
                        float h_neg = face_h_eff(tech, neg_bit, g->T[idx], T_env, lid_closed);
                        float h_rob = (seg_len == 1) ? h_neg : ((h_neg * (2.0f * k_i / ds)) / fmaxf(h_neg + 2.0f * k_i / ds, 1e-5f));
                        b[s] += h_rob;
                        d[s] += h_rob * T_env;
                    }
                    if (g->surface_mask[idx] & pos_bit) {
                        float h_pos = face_h_eff(tech, pos_bit, g->T[idx], T_env, lid_closed);
                        float h_rob = (seg_len == 1) ? h_pos : ((h_pos * (2.0f * k_i / ds)) / fmaxf(h_pos + 2.0f * k_i / ds, 1e-5f));
                        b[s] += h_rob;
                        d[s] += h_rob * T_env;
                    }
                }

                solve_tridiag_segment(seg_len, a, b, c, d, sol);
                for (int s = 0; s < seg_len; s++) {
                    g->T[map_idx[s]] = sol[s];
                }
            }
        }
    }
}

/* Sweep 1D implicit Fickian moisture diffusion along axis */
static void sweep_moisture_axis(fd_grid_t *g, int axis, float dt) {
    int n_axis = (axis == 0) ? g->nx : ((axis == 1) ? g->ny : g->nz);
    if (n_axis <= 1) return;
    int n_u = (axis == 0) ? g->ny : g->nx;
    int n_v = (axis == 2) ? g->ny : g->nz;
    float ds = (axis == 0) ? g->dx : ((axis == 1) ? g->dy : g->dz);
    float inv_ds2 = ((axis == 0) ? g->fiber_anisotropy : 1.0f) / (ds * ds);

    float a[FD_MAX_GRID_DIM], b[FD_MAX_GRID_DIM], c[FD_MAX_GRID_DIM], d[FD_MAX_GRID_DIM], sol[FD_MAX_GRID_DIM];
    int map_idx[FD_MAX_GRID_DIM];

    for (int v = 0; v < n_v; v++) {
        for (int u = 0; u < n_u; u++) {
            int i = 0;
            while (i < n_axis) {
                int x = (axis == 0) ? i : u;
                int y = (axis == 0) ? u : ((axis == 1) ? i : v);
                int z = (axis == 2) ? i : v;
                if (g->tissue[fd_grid_idx(g, x, y, z)] == FD_TISSUE_EMPTY) { i++; continue; }

                int seg_len = 0;
                while (i < n_axis) {
                    x = (axis == 0) ? i : u;
                    y = (axis == 0) ? u : ((axis == 1) ? i : v);
                    z = (axis == 2) ? i : v;
                    int idx = fd_grid_idx(g, x, y, z);
                    if (g->tissue[idx] == FD_TISSUE_EMPTY) break;
                    map_idx[seg_len++] = idx;
                    i++;
                }
                if (seg_len <= 1) continue;

                for (int s = 0; s < seg_len; s++) {
                    int idx = map_idx[s];
                    float tk = clampf(g->T[idx], 0.0f, 180.0f) + 273.15f;
                    float dw = 1.4e-9f * expf(-2150.0f * (1.0f / tk - 1.0f / 333.15f));
                    float r = dt * dw * inv_ds2;
                    float rl = (s > 0) ? r : 0.0f;
                    float rr = (s < seg_len - 1) ? r : 0.0f;
                    a[s] = -rl;
                    c[s] = -rr;
                    b[s] = 1.0f + rl + rr;
                    d[s] = g->W[idx];
                }
                solve_tridiag_segment(seg_len, a, b, c, d, sol);
                for (int s = 0; s < seg_len; s++) {
                    g->W[map_idx[s]] = clampf(sol[s], 0.005f, 0.98f);
                }
            }
        }
    }
}

void fd_physics_step(fd_sim_state_t *sim, const fd_database_t *db, float dt_sec) {
    if (!sim || !sim->initialized || !db || dt_sec <= 0.0f) return;
    fd_grid_t *g = &sim->grid;
    if (g->active_count <= 0) return;

    int tidx = (sim->active_tech_idx >= 0 && sim->active_tech_idx < db->technique_count) ? sim->active_tech_idx : 0;
    fd_technique_t fallback_tech = {
        .id = "pan_sear", .name = "Pan Sear", .h_bottom = 380.0f, .h_top = 14.0f, .h_side = 18.0f,
        .emissivity_rad = 0.0f, .ambient_humidity = 0.25f, .default_temp_c = 190.0f,
        .min_temp_c = 20.0f, .max_temp_c = 260.0f
    };
    const fd_technique_t *tech = (db->technique_count > 0) ? &db->techniques[tidx] : &fallback_tech;

    /* Sub-step large time warps to <= 2.5s so phase change & non-linear radiation stay accurate */
    int n_sub = (int)ceilf(dt_sec / 2.5f);
    if (n_sub < 1) n_sub = 1;
    if (n_sub > 40) n_sub = 40; /* shortcut: cap sub-steps at 40 per call, revisit if dt_sec > 100s */
    float dt = dt_sec / (float)n_sub;

    float raw_non_water = fmaxf(1.0f, sim->raw_weight_g);
    float x_prot  = sim->dry_protein_g / raw_non_water;
    float x_carb  = (sim->dry_starch_g + sim->dry_sugar_g) / raw_non_water;
    float x_sugar = sim->dry_sugar_g / raw_non_water;
    float x_fiber = sim->dry_fiber_g / raw_non_water;
    float x_ash   = sim->dry_ash_g / raw_non_water;

    float *rho_cp  = g->rho_cp;
    float *k_cond  = g->k_cond;
    float *rho_arr = g->rho_arr;

    float total_sizzle = 0.0f;
    float total_boil   = 0.0f;

    for (int step = 0; step < n_sub; step++) {
        /* 1. Compute Choi-Okos thermal properties and update ice/water fusion partition */
        for (int i = 0; i < g->count; i++) {
            if (g->tissue[i] == FD_TISSUE_EMPTY) {
                rho_cp[i] = 1000.0f; k_cond[i] = 0.5f; rho_arr[i] = 1000.0f;
                continue;
            }
            float w_tot = g->W[i] + g->ice[i];
            float freeze_f = (g->T[i] < -0.5f) ? clampf((-g->T[i] - 0.5f) / 3.5f, 0.0f, 0.92f) : 0.0f;
            g->ice[i] = w_tot * freeze_f;
            g->W[i]   = w_tot * (1.0f - freeze_f);

            float rho = 1000.0f, cp = 3500.0f, kc = 0.45f;
            float xp = (g->tissue[i] == FD_TISSUE_BONE) ? 0.22f : x_prot;
            float xa = (g->tissue[i] == FD_TISSUE_BONE) ? 0.45f : x_ash;
            fd_choi_okos(g->T[i], g->W[i], g->ice[i], xp, g->fat[i], x_carb, x_fiber, xa,
                         g->porosity[i], &rho, &cp, &kc);
            rho_arr[i] = rho;
            rho_cp[i]  = rho * cp;
            k_cond[i]  = kc;
        }

        /* 2. 3-pass unconditionally stable Thomas tridiagonal ADI along X, Y, Z */
        sweep_heat_axis(g, 0, dt, rho_cp, k_cond, tech, sim->ambient_temp_c, sim->lid_closed);
        sweep_heat_axis(g, 1, dt, rho_cp, k_cond, tech, sim->ambient_temp_c, sim->lid_closed);
        sweep_heat_axis(g, 2, dt, rho_cp, k_cond, tech, sim->ambient_temp_c, sim->lid_closed);

        /* 3. Boiling enthalpy clamp, surface desiccation evaporation, fat rendering & oil uptake */
        float cell_vol = g->dx * g->dy * g->dz;
        float evap_mass_step_kg = 0.0f;
        float oil_gain_step_kg  = 0.0f;

        for (int i = 0; i < g->count; i++) {
            if (g->tissue[i] == FD_TISSUE_EMPTY) continue;
            float aw = fd_compute_water_activity(g->W[i], g->salt[i], x_sugar);
            float t_boil = 100.0f + 2.5f * (1.0f - aw);
            float dw_evap = 0.0f;

            /* Psychrometric Lewis-analogy convective evaporation on air-exposed faces */
            if (g->surface_mask[i] != 0 && !tech->sealed_pouch && !tech->immersed_water && g->W[i] > 0.015f) {
                int air_faces = 0;
                if (g->count == 1) {
                    air_faces = 1; /* Liquid in vessel only evaporates from open +Z top surface */
                } else {
                    for (int b = 0; b < 6; b++) {
                        if ((1 << b) == FD_FACE_NEG_Z) continue;
                        if (g->surface_mask[i] & (1 << b)) air_faces++;
                    }
                }
                if (air_faces > 0) {
                    float rh = sim->lid_closed ? fmaxf(tech->ambient_humidity, 0.88f) : tech->ambient_humidity;
                    float w_sat_s = 0.0046f * expf(0.052f * clampf(g->T[i], 0.0f, 92.0f));
                    float w_amb   = sim->lid_closed ? (rh * w_sat_s) : (rh * 0.025f);
                    float drive   = fmaxf(0.0f, aw * w_sat_s - w_amb);
                    float g_mass  = ((float)air_faces * fmaxf(tech->h_top, 12.0f)) / (1006.0f * rho_arr[i] * fmaxf(g->dz, 0.004f));
                    dw_evap += fminf(g->W[i] - 0.012f, g_mass * drive * dt);

                    if (g->T[i] > 58.0f) {
                        float dT_cool = (dw_evap * rho_arr[i] * FD_LATENT_VAP_J_KG) / fmaxf(rho_cp[i], 1000.0f);
                        g->T[i] -= fminf(dT_cool, (g->T[i] - 58.0f) * 0.35f);
                    }
                }
                if ((g->surface_mask[i] & FD_FACE_NEG_Z) && sim->ambient_temp_c > 110.0f && g->T[i] > 85.0f) {
                    total_sizzle += 0.08f;
                }
            }

            /* Boiling plateau: pin temperature near ~100°C until free water desiccates into dry crust */
            if (g->T[i] > t_boil && g->W[i] > 0.055f && !tech->sealed_pouch) {
                float excess_j_m3 = rho_cp[i] * (g->T[i] - t_boil);
                float boil_dw = excess_j_m3 / (rho_arr[i] * FD_LATENT_VAP_J_KG);
                float max_boil = g->W[i] - 0.055f;
                if (boil_dw >= max_boil) {
                    /* Cell crosses dry-crust threshold; remaining enthalpy heats crust >100°C! */
                    float used_j = max_boil * rho_arr[i] * FD_LATENT_VAP_J_KG;
                    g->T[i] = t_boil + (excess_j_m3 - used_j) / rho_cp[i];
                    boil_dw = max_boil;
                } else {
                    g->T[i] = t_boil;
                }
                dw_evap += boil_dw;
                if (tech->immersed_water || g->nx == 1) total_boil += boil_dw * 800.0f;
                else total_sizzle += boil_dw * 500.0f;
            }

            /* Water hydration uptake when simmered/boiled in water with native/gelatinized starch */
            if (tech->immersed_water && x_carb > 0.15f && g->T[i] > 52.0f && g->W[i] < 0.72f) {
                float absorb = 0.0018f * dt * (0.72f - g->W[i]);
                g->W[i] += absorb;
                evap_mass_step_kg -= absorb * rho_arr[i] * cell_vol;
            }

            g->W[i] = clampf(g->W[i] - dw_evap, 0.008f, 0.96f);
            evap_mass_step_kg += dw_evap * rho_arr[i] * cell_vol;

            /* Subcutaneous fat rendering on hot exposed surfaces vs. frying capillary oil uptake */
            if (g->surface_mask[i] != 0) {
                if (tech->oil_uptake_rate > 0.0f && dw_evap > 0.0f) {
                    float d_oil = dw_evap * tech->oil_uptake_rate * 0.38f;
                    g->fat[i] = clampf(g->fat[i] + d_oil, 0.0f, 0.65f);
                    oil_gain_step_kg += d_oil * rho_arr[i] * cell_vol;
                } else if (g->tissue[i] == FD_TISSUE_FAT_CAP && g->T[i] > 68.0f && g->fat[i] > 0.20f) {
                    float render_df = 0.00045f * dt * ((g->T[i] - 68.0f) / 80.0f);
                    render_df = fminf(render_df, g->fat[i] - 0.20f);
                    g->fat[i] -= render_df;
                    oil_gain_step_kg -= render_df * rho_arr[i] * cell_vol;
                }
            }
        }

        /* 4. 3D Fickian internal moisture equilibration */
        sweep_moisture_axis(g, 0, dt);
        sweep_moisture_axis(g, 1, dt);
        sweep_moisture_axis(g, 2, dt);

        /* Scale voxel volume discrepancy to dish mass pool */
        float geom_vol_m3 = (float)g->active_count * cell_vol;
        float scale_mass = (geom_vol_m3 > 1e-9f) ? ((sim->raw_weight_g * 0.001f) / (geom_vol_m3 * 1020.0f)) : 1.0f;
        sim->total_water_g  = fmaxf(1.0f, sim->total_water_g - evap_mass_step_kg * 1000.0f * scale_mass);
        sim->absorbed_oil_g = sim->absorbed_oil_g + oil_gain_step_kg * 1000.0f * scale_mass;
    }

    sim->sizzle_flux = clampf(total_sizzle / (float)(n_sub * fmaxf(1, g->active_count / 8)), 0.0f, 1.0f);
    sim->boil_flux   = clampf(total_boil   / (float)(n_sub * fmaxf(1, g->active_count)), 0.0f, 1.0f);

    /* 5. Movement Shear (Foam Whipping & Emulsion Droplet Breakup) & Resting Reabsorption */
    float shear = tech->shear_rate + sim->shear_boost;
    sim->emulsion_shear_integral += shear * dt_sec;

    /* Off-heat resting myofibril juice reabsorption after thermal myosin denaturation */
    int cidx = fd_grid_idx(g, g->nx / 2, g->ny / 2, g->nz / 2);
    if (sim->ambient_temp_c <= 48.0f && g->denat_myosin[cidx] > 0.30f && x_prot > 0.08f) {
        float prev_rest = sim->rested_frac;
        sim->rested_frac = clampf(sim->rested_frac + (1.0f - sim->rested_frac) * (1.0f - expf(-dt_sec / 220.0f)), 0.0f, 1.0f);
        float d_rest = sim->rested_frac - prev_rest;
        if (d_rest > 0.0f) {
            for (int i = 0; i < g->count; i++) {
                if (g->tissue[i] != FD_TISSUE_EMPTY && g->tissue[i] != FD_TISSUE_BONE) {
                    g->W[i] = clampf(g->W[i] + 0.032f * d_rest, 0.008f, 0.96f);
                }
            }
            sim->total_water_g += 0.032f * d_rest * sim->raw_weight_g;
        }
    }

    /* Mechanical whipping incorporates air bubbles into protein/lecithin foams below coagulation */
    if (shear >= 45.0f && (sim->dry_protein_g > 1.2f || sim->emulsifier_units > 2.0f) && g->T[cidx] < 68.0f) {
        float fat_damp = clampf(1.0f - 0.55f * (sim->total_fat_g / fmaxf(10.0f, sim->raw_weight_g)), 0.25f, 1.0f);
        float d_foam = 0.000045f * shear * dt_sec * fat_damp * fmaxf(0.0f, 0.68f - sim->foam_air);
        sim->foam_air = clampf(sim->foam_air + d_foam, 0.0f, 0.70f);
        sim->whip_overwork += 0.0000022f * shear * dt_sec * fat_damp;
        if (sim->whip_overwork > 1.35f) sim->foam_air *= 0.92f;
        for (int i = 0; i < g->count; i++) {
            if (g->tissue[i] != FD_TISSUE_EMPTY) g->porosity[i] = clampf(fmaxf(g->porosity[i], sim->foam_air), 0.0f, 0.78f);
        }
    }

    /* 6. Krieger-Dougherty Emulsion Rheology & Stability (liquid sauces & starch-emulsified pasta) */
    float tot_liq = sim->total_water_g + fmaxf(0.0f, sim->total_fat_g + sim->absorbed_oil_g);
    float phi_oil = (tot_liq > 1.0f) ? clampf((sim->total_fat_g + sim->absorbed_oil_g) / tot_liq, 0.0f, 0.95f) : 0.0f;
    float raw_starch_shield = g->starch_gel[cidx] * (sim->dry_starch_g / fmaxf(sim->raw_weight_g, 1.0f)) * 4.5f;
    float starch_shield = (g->count == 1) ? raw_starch_shield : fminf(raw_starch_shield, 0.20f);
    float emul_cap = (sim->emulsifier_units / fmaxf(10.0f, sim->total_fat_g + 5.0f)) + starch_shield;

    float stab = 1.0f;
    if ((g->count == 1 || sim->emulsifier_units > 2.5f) && phi_oil > 0.08f && (1.0f - phi_oil) > 0.08f) {
        int bot_idx = fd_grid_idx(g, g->nx / 2, g->ny / 2, 0);
        if (g->tissue[bot_idx] == FD_TISSUE_EMPTY) bot_idx = cidx;
        float shear_bonus = clampf(sim->emulsion_shear_integral / 900.0f, 0.0f, 0.14f);
        float phi_max = clampf(0.28f + 0.54f * clampf(emul_cap, 0.0f, 1.2f) + shear_bonus, 0.28f, 0.86f);
        if (shear < 5.0f && sim->emulsion_shear_integral < 320.0f && emul_cap < 0.45f) stab *= 0.35f;
        if (phi_oil > phi_max) stab *= clampf(1.0f - 3.5f * (phi_oil - phi_max), 0.05f, 1.0f);
        /* Thermal curdling of egg/dairy emulsifier (>72.5°C unshielded, >77.5°C with pasta starch shield) */
        float t_emul = (g->count == 1) ? g->T[0] : g->T[bot_idx];
        float t_curdle = 72.5f + 25.0f * fminf(starch_shield, 0.20f);
        if (t_emul > t_curdle && raw_starch_shield < 0.45f && sim->emulsifier_units > 0.1f && g->count == 1) {
            stab *= clampf(1.0f - 0.15f * (t_emul - 72.5f), 0.05f, 1.0f);
        } else if (g->count > 1 && t_emul > t_curdle && sim->emulsifier_units > 2.5f) {
            stab *= clampf(1.0f - 0.12f * (t_emul - t_curdle), 0.08f, 1.0f);
        }
        /* Irreversible egg yolk coagulation: once scrambled, cooling cannot restore lecithin film */
        if (g->denat_albumen[bot_idx] > 0.65f && raw_starch_shield < 0.25f && sim->emulsifier_units > 0.5f) {
            stab *= 0.28f;
        }
        /* Acid curdling near casein isoelectric point pH 4.6 without strong lecithin/starch */
        if (sim->metrics.ph > 0.1f && sim->metrics.ph < 4.7f && starch_shield < 0.08f && sim->emulsifier_units < 12.0f) {
            stab *= 0.45f;
        }
    }
    sim->metrics.emulsion_stability = clampf(stab, 0.0f, 1.0f);
    float ratio = fminf(0.94f, phi_oil / 0.80f);
    sim->metrics.emulsion_viscosity_cp = 1.2f * powf(1.0f - ratio, -2.0f) * (1.0f + 12.0f * starch_shield);
}
