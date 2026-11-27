#ifndef FD_GEOMETRY_H
#define FD_GEOMETRY_H

#include "fd_types.h"

typedef enum {
    FD_SLICE_XY = 0,
    FD_SLICE_XZ = 1,
    FD_SLICE_YZ = 2
} fd_slice_plane_t;

typedef enum {
    FD_OVERLAY_TEMP       = 0,
    FD_OVERLAY_MOISTURE   = 1,
    FD_OVERLAY_MAILLARD   = 2,
    FD_OVERLAY_PROTEIN    = 3,
    FD_OVERLAY_LETHALITY  = 4
} fd_overlay_mode_t;

void fd_geometry_choose_dims(fd_shape_type_t shape, float lx_mm, float ly_mm, float lz_mm,
                             bool coarse_mode, int *out_nx, int *out_ny, int *out_nz);

bool fd_grid_alloc(fd_grid_t *g, int nx, int ny, int nz, float lx_mm, float ly_mm, float lz_mm);
void fd_grid_free(fd_grid_t *g);

void fd_grid_rasterize(fd_grid_t *g, fd_shape_type_t shape,
                       float init_temp_c, float init_water, float init_fat, float init_salt,
                       float fiber_anisotropy);

void fd_grid_update_surfaces(fd_grid_t *g);
void fd_grid_flip_z(fd_grid_t *g);
void fd_grid_toss_cubes(fd_grid_t *g);
bool fd_grid_resize_preserve(fd_grid_t *g, fd_shape_type_t new_shape,
                             float lx_mm, float ly_mm, float lz_mm, bool coarse_mode);

/* Extract a 2D orthogonal cross-section slice into RGBA8888 pixels (max 32x32) */
void fd_grid_extract_slice(const fd_grid_t *g, fd_slice_plane_t plane, int slice_idx,
                           fd_overlay_mode_t mode, uint8_t *rgba_out, int *out_w, int *out_h);

static inline int fd_grid_idx(const fd_grid_t *g, int x, int y, int z) {
    return (z * g->ny + y) * g->nx + x;
}

#endif /* FD_GEOMETRY_H */
