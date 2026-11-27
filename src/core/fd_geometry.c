#include "fd_geometry.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void fd_geometry_choose_dims(fd_shape_type_t shape, float lx_mm, float ly_mm, float lz_mm,
                             bool coarse_mode, int *out_nx, int *out_ny, int *out_nz) {
    if (shape == FD_SHAPE_LIQUID) {
        *out_nx = 1; *out_ny = 1; *out_nz = 1;
        return;
    }
    if (coarse_mode) {
        *out_nx = 8;
        *out_ny = 8;
        *out_nz = (lz_mm < 0.5f * lx_mm) ? 6 : 8;
        return;
    }
    if (lz_mm < 0.55f * lx_mm) {
        /* Flat cut (steak, fillet, pancake): prioritize vertical crust-to-core resolution */
        *out_nx = 26; *out_ny = 20; *out_nz = 16;
    } else {
        *out_nx = 22; *out_ny = 22; *out_nz = 22;
    }
    (void)ly_mm;
}

bool fd_grid_alloc(fd_grid_t *g, int nx, int ny, int nz, float lx_mm, float ly_mm, float lz_mm) {
    if (!g) return false;
    fd_grid_free(g);
    nx = clamp_int(nx, 1, FD_MAX_GRID_DIM);
    ny = clamp_int(ny, 1, FD_MAX_GRID_DIM);
    nz = clamp_int(nz, 1, FD_MAX_GRID_DIM);
    int n = nx * ny * nz;

    g->nx = nx; g->ny = ny; g->nz = nz;
    g->count = n;
    g->lx = fmaxf(lx_mm, 2.0f) * 0.001f;
    g->ly = fmaxf(ly_mm, 2.0f) * 0.001f;
    g->lz = fmaxf(lz_mm, 2.0f) * 0.001f;
    g->dx = g->lx / (float)nx;
    g->dy = g->ly / (float)ny;
    g->dz = g->lz / (float)nz;
    g->fiber_anisotropy = 1.0f;

    g->tissue          = (uint8_t *)calloc((size_t)n, sizeof(uint8_t));
    g->surface_mask    = (uint8_t *)calloc((size_t)n, sizeof(uint8_t));
    g->T               = (float *)calloc((size_t)n, sizeof(float));
    g->W               = (float *)calloc((size_t)n, sizeof(float));
    g->ice             = (float *)calloc((size_t)n, sizeof(float));
    g->fat             = (float *)calloc((size_t)n, sizeof(float));
    g->maillard        = (float *)calloc((size_t)n, sizeof(float));
    g->caramel         = (float *)calloc((size_t)n, sizeof(float));
    g->char_level      = (float *)calloc((size_t)n, sizeof(float));
    g->denat_myosin    = (float *)calloc((size_t)n, sizeof(float));
    g->denat_actin     = (float *)calloc((size_t)n, sizeof(float));
    g->denat_albumen   = (float *)calloc((size_t)n, sizeof(float));
    g->gelatin         = (float *)calloc((size_t)n, sizeof(float));
    g->starch_gel      = (float *)calloc((size_t)n, sizeof(float));
    g->starch_retro    = (float *)calloc((size_t)n, sizeof(float));
    g->porosity        = (float *)calloc((size_t)n, sizeof(float));
    g->salt            = (float *)calloc((size_t)n, sizeof(float));
    g->lethality_log10 = (float *)calloc((size_t)n, sizeof(float));
    g->rho_arr         = (float *)calloc((size_t)n, sizeof(float));
    g->rho_cp          = (float *)calloc((size_t)n, sizeof(float));
    g->k_cond          = (float *)calloc((size_t)n, sizeof(float));

    if (!g->tissue || !g->surface_mask || !g->T || !g->W || !g->ice || !g->fat ||
        !g->maillard || !g->caramel || !g->char_level || !g->denat_myosin ||
        !g->denat_actin || !g->denat_albumen || !g->gelatin || !g->starch_gel ||
        !g->starch_retro || !g->porosity || !g->salt || !g->lethality_log10 ||
        !g->rho_arr || !g->rho_cp || !g->k_cond) {
        fd_grid_free(g);
        return false;
    }
    return true;
}

void fd_grid_free(fd_grid_t *g) {
    if (!g) return;
    free(g->tissue);          free(g->surface_mask);
    free(g->T);               free(g->W);
    free(g->ice);
    free(g->fat);             free(g->maillard);
    free(g->caramel);         free(g->char_level);
    free(g->denat_myosin);    free(g->denat_actin);
    free(g->denat_albumen);   free(g->gelatin);
    free(g->starch_gel);      free(g->starch_retro);
    free(g->porosity);        free(g->salt);
    free(g->lethality_log10);
    free(g->rho_arr);         free(g->rho_cp);
    free(g->k_cond);
    memset(g, 0, sizeof(*g));
}

void fd_grid_update_surfaces(fd_grid_t *g) {
    if (!g || !g->tissue) return;
    int active = 0;
    for (int z = 0; z < g->nz; z++) {
        for (int y = 0; y < g->ny; y++) {
            for (int x = 0; x < g->nx; x++) {
                int i = fd_grid_idx(g, x, y, z);
                if (g->tissue[i] == FD_TISSUE_EMPTY) {
                    g->surface_mask[i] = 0;
                    continue;
                }
                active++;
                uint8_t m = 0;
                if (x == 0         || g->tissue[fd_grid_idx(g, x - 1, y, z)] == FD_TISSUE_EMPTY) m |= FD_FACE_NEG_X;
                if (x == g->nx - 1 || g->tissue[fd_grid_idx(g, x + 1, y, z)] == FD_TISSUE_EMPTY) m |= FD_FACE_POS_X;
                if (y == 0         || g->tissue[fd_grid_idx(g, x, y - 1, z)] == FD_TISSUE_EMPTY) m |= FD_FACE_NEG_Y;
                if (y == g->ny - 1 || g->tissue[fd_grid_idx(g, x, y + 1, z)] == FD_TISSUE_EMPTY) m |= FD_FACE_POS_Y;
                if (z == 0         || g->tissue[fd_grid_idx(g, x, y, z - 1)] == FD_TISSUE_EMPTY) m |= FD_FACE_NEG_Z;
                if (z == g->nz - 1 || g->tissue[fd_grid_idx(g, x, y, z + 1)] == FD_TISSUE_EMPTY) m |= FD_FACE_POS_Z;
                g->surface_mask[i] = m;
            }
        }
    }
    g->active_count = active;
}

void fd_grid_rasterize(fd_grid_t *g, fd_shape_type_t shape,
                       float init_temp_c, float init_water, float init_fat, float init_salt,
                       float fiber_anisotropy) {
    if (!g || !g->tissue) return;
    g->fiber_anisotropy = fmaxf(1.0f, fiber_anisotropy);
    float frozen_frac = (init_temp_c < -1.0f) ? clampf((-init_temp_c - 1.0f) / 4.0f, 0.0f, 0.9f) : 0.0f;

    for (int z = 0; z < g->nz; z++) {
        float uz = (g->nz > 1) ? ((z + 0.5f) / (float)g->nz) : 0.5f;
        float nz_c = uz * 2.0f - 1.0f;
        for (int y = 0; y < g->ny; y++) {
            float uy = (g->ny > 1) ? ((y + 0.5f) / (float)g->ny) : 0.5f;
            float ny_c = uy * 2.0f - 1.0f;
            for (int x = 0; x < g->nx; x++) {
                float ux = (g->nx > 1) ? ((x + 0.5f) / (float)g->nx) : 0.5f;
                float nx_c = ux * 2.0f - 1.0f;
                int idx = fd_grid_idx(g, x, y, z);

                uint8_t tis = FD_TISSUE_LEAN;
                float w = init_water;
                float f = init_fat;

                switch (shape) {
                    case FD_SHAPE_LIQUID:
                    case FD_SHAPE_SLAB:
                    case FD_SHAPE_CUBE:
                        tis = FD_TISSUE_LEAN;
                        break;
                    case FD_SHAPE_CYLINDER:
                        if (nx_c * nx_c + ny_c * ny_c > 1.02f) tis = FD_TISSUE_EMPTY;
                        break;
                    case FD_SHAPE_SPHERE:
                        if (nx_c * nx_c + ny_c * ny_c + nz_c * nz_c > 1.04f) tis = FD_TISSUE_EMPTY;
                        break;
                    case FD_SHAPE_TAPERED_FILLET: {
                        float max_uz = 1.0f - 0.58f * ux; /* Tapers from 100% head to 42% tail */
                        if (uz > max_uz) {
                            tis = FD_TISSUE_EMPTY;
                        } else if (z == 0 && g->nz >= 4) {
                            tis = FD_TISSUE_SKIN;
                            w = init_water * 0.65f;
                            f = fminf(0.45f, init_fat + 0.18f);
                        }
                        break;
                    }
                    case FD_SHAPE_FAT_CAP_SLAB:
                        if (z == 0 && g->nz >= 4) {
                            tis = FD_TISSUE_SKIN;
                            w = 0.42f; f = 0.38f;
                        } else if (uz < 0.28f && g->nz >= 4) {
                            tis = FD_TISSUE_FAT_CAP;
                            w = 0.18f; f = 0.72f;
                        }
                        break;
                    case FD_SHAPE_BONE_IN_SLAB:
                        if (ux < 0.18f && g->nx >= 4) {
                            tis = FD_TISSUE_BONE;
                            w = 0.15f; f = 0.12f;
                        }
                        break;
                    case FD_SHAPE_CONCENTRIC_EGG: {
                        float r2 = nx_c * nx_c + ny_c * ny_c + nz_c * nz_c;
                        if (r2 > 1.04f) {
                            tis = FD_TISSUE_EMPTY;
                        } else if (r2 <= 0.32f) {
                            tis = FD_TISSUE_YOLK_CORE;
                            w = 0.51f; f = 0.27f;
                        } else {
                            tis = FD_TISSUE_LEAN;
                            w = 0.88f; f = 0.002f;
                        }
                        break;
                    }
                }

                g->tissue[idx] = tis;
                g->T[idx]      = init_temp_c;
                g->W[idx]      = (tis == FD_TISSUE_EMPTY) ? 0.0f : w * (1.0f - frozen_frac);
                g->ice[idx]    = (tis == FD_TISSUE_EMPTY) ? 0.0f : w * frozen_frac;
                g->fat[idx]    = (tis == FD_TISSUE_EMPTY) ? 0.0f : f;
                g->salt[idx]   = (tis == FD_TISSUE_EMPTY) ? 0.0f : init_salt;
                g->maillard[idx]        = 0.0f;
                g->caramel[idx]         = 0.0f;
                g->char_level[idx]      = 0.0f;
                g->denat_myosin[idx]    = 0.0f;
                g->denat_actin[idx]     = 0.0f;
                g->denat_albumen[idx]   = 0.0f;
                g->gelatin[idx]         = 0.0f;
                g->starch_gel[idx]      = 0.0f;
                g->starch_retro[idx]    = 0.0f;
                g->porosity[idx]        = 0.0f;
                g->lethality_log10[idx] = 0.0f;
            }
        }
    }
    fd_grid_update_surfaces(g);
}

#define SWAP_VAL(type, a, b) do { type _t = (a); (a) = (b); (b) = _t; } while (0)

static void swap_voxels(fd_grid_t *g, int i, int j) {
    SWAP_VAL(uint8_t, g->tissue[i],          g->tissue[j]);
    SWAP_VAL(float,   g->T[i],               g->T[j]);
    SWAP_VAL(float,   g->W[i],               g->W[j]);
    SWAP_VAL(float,   g->ice[i],             g->ice[j]);
    SWAP_VAL(float,   g->fat[i],             g->fat[j]);
    SWAP_VAL(float,   g->maillard[i],        g->maillard[j]);
    SWAP_VAL(float,   g->caramel[i],         g->caramel[j]);
    SWAP_VAL(float,   g->char_level[i],      g->char_level[j]);
    SWAP_VAL(float,   g->denat_myosin[i],    g->denat_myosin[j]);
    SWAP_VAL(float,   g->denat_actin[i],     g->denat_actin[j]);
    SWAP_VAL(float,   g->denat_albumen[i],   g->denat_albumen[j]);
    SWAP_VAL(float,   g->gelatin[i],         g->gelatin[j]);
    SWAP_VAL(float,   g->starch_gel[i],      g->starch_gel[j]);
    SWAP_VAL(float,   g->starch_retro[i],    g->starch_retro[j]);
    SWAP_VAL(float,   g->porosity[i],        g->porosity[j]);
    SWAP_VAL(float,   g->salt[i],            g->salt[j]);
    SWAP_VAL(float,   g->lethality_log10[i], g->lethality_log10[j]);
}

void fd_grid_flip_z(fd_grid_t *g) {
    if (!g || g->nz <= 1) return;
    for (int z = 0; z < g->nz / 2; z++) {
        int z2 = g->nz - 1 - z;
        for (int y = 0; y < g->ny; y++) {
            for (int x = 0; x < g->nx; x++) {
                swap_voxels(g, fd_grid_idx(g, x, y, z), fd_grid_idx(g, x, y, z2));
            }
        }
    }
    fd_grid_update_surfaces(g);
}

void fd_grid_toss_cubes(fd_grid_t *g) {
    /* Rotate Z->Y->X face exposure for stir-fry / sauté cube tossing when cubic */
    if (!g || g->nx != g->nz || g->nx <= 1) {
        fd_grid_flip_z(g);
        return;
    }
    int n = g->nx;
    for (int y = 0; y < g->ny; y++) {
        for (int z = 0; z < n; z++) {
            for (int x = z + 1; x < n; x++) {
                swap_voxels(g, fd_grid_idx(g, x, y, z), fd_grid_idx(g, z, y, x));
            }
        }
    }
    fd_grid_flip_z(g);
}

bool fd_grid_resize_preserve(fd_grid_t *g, fd_shape_type_t new_shape,
                             float lx_mm, float ly_mm, float lz_mm, bool coarse_mode) {
    if (!g || !g->tissue || g->active_count <= 0) return false;
    float aniso = g->fiber_anisotropy;
    float avg_t = 0.0f, avg_w = 0.0f, avg_fat = 0.0f, avg_salt = 0.0f;
    float avg_mail = 0.0f, avg_cara = 0.0f, avg_char = 0.0f;
    float avg_myo = 0.0f, avg_act = 0.0f, avg_alb = 0.0f, avg_gel = 0.0f;
    float avg_sg = 0.0f, avg_sr = 0.0f, avg_por = 0.0f, min_leth = 1e9f;
    int cnt = 0;

    for (int i = 0; i < g->count; i++) {
        if (g->tissue[i] == FD_TISSUE_EMPTY) continue;
        avg_t    += g->T[i];
        avg_w    += g->W[i] + g->ice[i];
        avg_fat  += g->fat[i];
        avg_salt += g->salt[i];
        avg_mail += g->maillard[i];
        avg_cara += g->caramel[i];
        avg_char += g->char_level[i];
        avg_myo  += g->denat_myosin[i];
        avg_act  += g->denat_actin[i];
        avg_alb  += g->denat_albumen[i];
        avg_gel  += g->gelatin[i];
        avg_sg   += g->starch_gel[i];
        avg_sr   += g->starch_retro[i];
        avg_por  += g->porosity[i];
        if (g->tissue[i] != FD_TISSUE_BONE && g->lethality_log10[i] < min_leth) {
            min_leth = g->lethality_log10[i];
        }
        cnt++;
    }
    if (cnt <= 0) return false;
    float inv = 1.0f / (float)cnt;
    if (min_leth > 1e8f) min_leth = 0.0f;

    int nx = 1, ny = 1, nz = 1;
    fd_geometry_choose_dims(new_shape, lx_mm, ly_mm, lz_mm, coarse_mode, &nx, &ny, &nz);
    if (!fd_grid_alloc(g, nx, ny, nz, lx_mm, ly_mm, lz_mm)) return false;
    fd_grid_rasterize(g, new_shape, avg_t * inv, avg_w * inv, avg_fat * inv, avg_salt * inv, aniso);

    for (int i = 0; i < g->count; i++) {
        if (g->tissue[i] == FD_TISSUE_EMPTY) continue;
        g->maillard[i]        = avg_mail * inv;
        g->caramel[i]         = avg_cara * inv;
        g->char_level[i]      = avg_char * inv;
        g->denat_myosin[i]    = avg_myo * inv;
        g->denat_actin[i]     = avg_act * inv;
        g->denat_albumen[i]   = avg_alb * inv;
        g->gelatin[i]         = avg_gel * inv;
        g->starch_gel[i]      = avg_sg * inv;
        g->starch_retro[i]    = avg_sr * inv;
        g->porosity[i]        = avg_por * inv;
        g->lethality_log10[i] = min_leth;
    }
    return true;
}

static void color_ramp(float u, uint8_t r0, uint8_t g0, uint8_t b0,
                       uint8_t r1, uint8_t g1, uint8_t b1, uint8_t *out) {
    u = clampf(u, 0.0f, 1.0f);
    out[0] = (uint8_t)(r0 + u * (float)(r1 - r0));
    out[1] = (uint8_t)(g0 + u * (float)(g1 - g0));
    out[2] = (uint8_t)(b0 + u * (float)(b1 - b0));
    out[3] = 255;
}

void fd_grid_extract_slice(const fd_grid_t *g, fd_slice_plane_t plane, int slice_idx,
                           fd_overlay_mode_t mode, uint8_t *rgba_out, int *out_w, int *out_h) {
    if (!g || !rgba_out || !out_w || !out_h) return;
    int w = g->nx, h = g->ny;
    if (plane == FD_SLICE_XY) {
        w = g->nx; h = g->ny;
        slice_idx = clamp_int(slice_idx, 0, g->nz - 1);
    } else if (plane == FD_SLICE_XZ) {
        w = g->nx; h = g->nz;
        slice_idx = clamp_int(slice_idx, 0, g->ny - 1);
    } else {
        w = g->ny; h = g->nz;
        slice_idx = clamp_int(slice_idx, 0, g->nx - 1);
    }
    *out_w = w;
    *out_h = h;

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int x = i, y = j, z = slice_idx;
            if (plane == FD_SLICE_XZ) {
                x = i; y = slice_idx; z = h - 1 - j; /* z=0 bottom at bottom of image */
            } else if (plane == FD_SLICE_YZ) {
                x = slice_idx; y = i; z = h - 1 - j;
            }
            int idx = fd_grid_idx(g, x, y, z);
            uint8_t *px = &rgba_out[(j * w + i) * 4];
            if (g->tissue[idx] == FD_TISSUE_EMPTY) {
                px[0] = 22; px[1] = 25; px[2] = 32; px[3] = 255;
                continue;
            }
            if (g->tissue[idx] == FD_TISSUE_BONE && mode != FD_OVERLAY_TEMP) {
                px[0] = 224; px[1] = 218; px[2] = 204; px[3] = 255;
                continue;
            }
            switch (mode) {
                case FD_OVERLAY_TEMP: {
                    float t = g->T[idx];
                    if (t < 55.0f) {
                        color_ramp((t + 5.0f) / 60.0f, 38, 92, 210, 230, 88, 98, px);
                    } else if (t < 110.0f) {
                        color_ramp((t - 55.0f) / 55.0f, 230, 88, 98, 245, 200, 66, px);
                    } else {
                        color_ramp((t - 110.0f) / 90.0f, 245, 200, 66, 255, 250, 220, px);
                    }
                    break;
                }
                case FD_OVERLAY_MOISTURE:
                    color_ramp(g->W[idx] / 0.85f, 168, 118, 62, 52, 164, 235, px);
                    break;
                case FD_OVERLAY_MAILLARD: {
                    float b = clampf(g->maillard[idx] + 0.7f * g->caramel[idx], 0.0f, 1.0f);
                    if (g->char_level[idx] > 0.15f) {
                        color_ramp(g->char_level[idx], 125, 68, 28, 25, 20, 18, px);
                    } else {
                        color_ramp(b, 236, 182, 158, 122, 58, 22, px);
                    }
                    break;
                }
                case FD_OVERLAY_PROTEIN: {
                    float d = fmaxf(g->denat_myosin[idx], g->starch_gel[idx]);
                    color_ramp(d, 218, 72, 92, 235, 222, 188, px);
                    break;
                }
                case FD_OVERLAY_LETHALITY:
                    color_ramp(g->lethality_log10[idx] / 7.0f, 215, 58, 68, 44, 198, 118, px);
                    break;
            }
        }
    }
}
