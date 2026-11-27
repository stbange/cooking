#ifndef FD_GL_RENDERER_H
#define FD_GL_RENDERER_H

#include "../core/fd_ui.h"
#include "../core/fd_vfs.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t prog;
    uint32_t vao;
    uint32_t vbo;
    int32_t  u_screen_loc;
    int32_t  u_tex_loc;
    uint32_t textures[3]; /* FD_TEX_FONT, FD_TEX_KITCHEN, FD_TEX_SLICE */
    bool     ready;
} fd_gl_renderer_t;

bool fd_gl_init(fd_gl_renderer_t *r, const fd_vfs_t *vfs);
void fd_gl_free(fd_gl_renderer_t *r);

void fd_gl_update_slice_tex(fd_gl_renderer_t *r, const uint8_t *rgba, int w, int h);
void fd_gl_render(fd_gl_renderer_t *r, const fd_ui_t *ui, int vp_w, int vp_h);

#endif /* FD_GL_RENDERER_H */
