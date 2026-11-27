#include "gl_renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"

#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <SDL3/SDL_opengl.h>

/* Minimal GL 3.3 Core entry points loaded dynamically via SDL_GL_GetProcAddress */
static PFNGLCREATESHADERPROC            p_glCreateShader;
static PFNGLSHADERSOURCEPROC            p_glShaderSource;
static PFNGLCOMPILESHADERPROC           p_glCompileShader;
static PFNGLGETSHADERIVPROC             p_glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC        p_glGetShaderInfoLog;
static PFNGLDELETESHADERPROC            p_glDeleteShader;
static PFNGLCREATEPROGRAMPROC           p_glCreateProgram;
static PFNGLATTACHSHADERPROC            p_glAttachShader;
static PFNGLLINKPROGRAMPROC             p_glLinkProgram;
static PFNGLGETPROGRAMIVPROC            p_glGetProgramiv;
static PFNGLUSEPROGRAMPROC              p_glUseProgram;
static PFNGLDELETEPROGRAMPROC           p_glDeleteProgram;
static PFNGLGETUNIFORMLOCATIONPROC      p_glGetUniformLocation;
static PFNGLUNIFORM2FPROC               p_glUniform2f;
static PFNGLUNIFORM1IPROC               p_glUniform1i;
static PFNGLGENVERTEXARRAYSPROC         p_glGenVertexArrays;
static PFNGLBINDVERTEXARRAYPROC         p_glBindVertexArray;
static PFNGLDELETEVERTEXARRAYSPROC      p_glDeleteVertexArrays;
static PFNGLGENBUFFERSPROC              p_glGenBuffers;
static PFNGLBINDBUFFERPROC              p_glBindBuffer;
static PFNGLBUFFERDATAPROC              p_glBufferData;
static PFNGLDELETEBUFFERSPROC           p_glDeleteBuffers;
static PFNGLENABLEVERTEXATTRIBARRAYPROC p_glEnableVertexAttribArray;
static PFNGLVERTEXATTRIBPOINTERPROC     p_glVertexAttribPointer;
static PFNGLACTIVETEXTUREPROC           p_glActiveTexture;

static bool load_gl_procs(void) {
#define LOAD_GL(fn, type) do { \
    p_##fn = (type)SDL_GL_GetProcAddress(#fn); \
    if (!p_##fn) return false; \
} while (0)
    LOAD_GL(glCreateShader,            PFNGLCREATESHADERPROC);
    LOAD_GL(glShaderSource,            PFNGLSHADERSOURCEPROC);
    LOAD_GL(glCompileShader,           PFNGLCOMPILESHADERPROC);
    LOAD_GL(glGetShaderiv,             PFNGLGETSHADERIVPROC);
    LOAD_GL(glGetShaderInfoLog,        PFNGLGETSHADERINFOLOGPROC);
    LOAD_GL(glDeleteShader,            PFNGLDELETESHADERPROC);
    LOAD_GL(glCreateProgram,           PFNGLCREATEPROGRAMPROC);
    LOAD_GL(glAttachShader,            PFNGLATTACHSHADERPROC);
    LOAD_GL(glLinkProgram,             PFNGLLINKPROGRAMPROC);
    LOAD_GL(glGetProgramiv,            PFNGLGETPROGRAMIVPROC);
    LOAD_GL(glUseProgram,              PFNGLUSEPROGRAMPROC);
    LOAD_GL(glDeleteProgram,           PFNGLDELETEPROGRAMPROC);
    LOAD_GL(glGetUniformLocation,      PFNGLGETUNIFORMLOCATIONPROC);
    LOAD_GL(glUniform2f,               PFNGLUNIFORM2FPROC);
    LOAD_GL(glUniform1i,               PFNGLUNIFORM1IPROC);
    LOAD_GL(glGenVertexArrays,         PFNGLGENVERTEXARRAYSPROC);
    LOAD_GL(glBindVertexArray,         PFNGLBINDVERTEXARRAYPROC);
    LOAD_GL(glDeleteVertexArrays,      PFNGLDELETEVERTEXARRAYSPROC);
    LOAD_GL(glGenBuffers,              PFNGLGENBUFFERSPROC);
    LOAD_GL(glBindBuffer,              PFNGLBINDBUFFERPROC);
    LOAD_GL(glBufferData,              PFNGLBUFFERDATAPROC);
    LOAD_GL(glDeleteBuffers,           PFNGLDELETEBUFFERSPROC);
    LOAD_GL(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    LOAD_GL(glVertexAttribPointer,     PFNGLVERTEXATTRIBPOINTERPROC);
    LOAD_GL(glActiveTexture,           PFNGLACTIVETEXTUREPROC);
#undef LOAD_GL
    return true;
}

#define glCreateShader            p_glCreateShader
#define glShaderSource            p_glShaderSource
#define glCompileShader           p_glCompileShader
#define glGetShaderiv             p_glGetShaderiv
#define glGetShaderInfoLog        p_glGetShaderInfoLog
#define glDeleteShader            p_glDeleteShader
#define glCreateProgram           p_glCreateProgram
#define glAttachShader            p_glAttachShader
#define glLinkProgram             p_glLinkProgram
#define glGetProgramiv            p_glGetProgramiv
#define glUseProgram              p_glUseProgram
#define glDeleteProgram           p_glDeleteProgram
#define glGetUniformLocation      p_glGetUniformLocation
#define glUniform2f               p_glUniform2f
#define glUniform1i               p_glUniform1i
#define glGenVertexArrays         p_glGenVertexArrays
#define glBindVertexArray         p_glBindVertexArray
#define glDeleteVertexArrays      p_glDeleteVertexArrays
#define glGenBuffers              p_glGenBuffers
#define glBindBuffer              p_glBindBuffer
#define glBufferData              p_glBufferData
#define glDeleteBuffers           p_glDeleteBuffers
#define glEnableVertexAttribArray p_glEnableVertexAttribArray
#define glVertexAttribPointer     p_glVertexAttribPointer
#define glActiveTexture           p_glActiveTexture
#endif

static uint32_t compile_shader_vfs(const fd_vfs_t *vfs, const char *rel_path, GLenum stage) {
    uint8_t *body = NULL;
    size_t sz = 0;
    if (!fd_vfs_read(vfs, rel_path, &body, &sz)) return 0;

#ifdef __EMSCRIPTEN__
    const char *preamble = "#version 300 es\nprecision mediump float;\n";
#else
    const char *preamble = "#version 330 core\n";
#endif
    const GLchar *srcs[2] = { preamble, (const GLchar *)body };
    GLuint sh = glCreateShader(stage);
    glShaderSource(sh, 2, srcs, NULL);
    glCompileShader(sh);
    free(body);

    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        SDL_Log("GL shader error (%s): %s", rel_path, log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static uint32_t load_png_texture(const fd_vfs_t *vfs, const char *rel_path, GLint filter) {
    uint8_t *raw = NULL;
    size_t sz = 0;
    if (!fd_vfs_read(vfs, rel_path, &raw, &sz)) return 0;

    int w = 0, h = 0, comp = 0;
    stbi_uc *px = stbi_load_from_memory(raw, (int)sz, &w, &h, &comp, 4);
    free(raw);
    if (!px) return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);
    return tex;
}

static uint32_t create_dynamic_tex(GLint filter) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

bool fd_gl_init(fd_gl_renderer_t *r, const fd_vfs_t *vfs) {
    if (!r || !vfs) return false;
    memset(r, 0, sizeof(*r));
#ifndef __EMSCRIPTEN__
    if (!load_gl_procs()) return false;
#endif

    uint32_t vs = compile_shader_vfs(vfs, "shaders/quad_ui.vert", GL_VERTEX_SHADER);
    uint32_t fs = compile_shader_vfs(vfs, "shaders/quad_ui.frag", GL_FRAGMENT_SHADER);
    if (!vs || !fs) return false;

    r->prog = glCreateProgram();
    glAttachShader(r->prog, vs);
    glAttachShader(r->prog, fs);
    glLinkProgram(r->prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(r->prog, GL_LINK_STATUS, &linked);
    if (!linked) return false;

    r->u_screen_loc = glGetUniformLocation(r->prog, "u_screen");
    r->u_tex_loc    = glGetUniformLocation(r->prog, "u_tex");

    glGenVertexArrays(1, &r->vao);
    glBindVertexArray(r->vao);
    glGenBuffers(1, &r->vbo);
    glGenBuffers(1, &r->ebo);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(fd_vertex_t), (const void *)offsetof(fd_vertex_t, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(fd_vertex_t), (const void *)offsetof(fd_vertex_t, u));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(fd_vertex_t), (const void *)offsetof(fd_vertex_t, r));

    r->textures[FD_TEX_FONT]    = create_dynamic_tex(GL_LINEAR);
    r->textures[FD_TEX_KITCHEN] = load_png_texture(vfs, "graphics/kitchen_atlas.png", GL_LINEAR);
    r->textures[FD_TEX_SLICE]   = create_dynamic_tex(GL_NEAREST);
    r->textures[FD_TEX_ICONS]   = load_png_texture(vfs, "graphics/icons.png", GL_LINEAR);

    r->ready = (r->textures[FD_TEX_FONT] != 0 && r->textures[FD_TEX_KITCHEN] != 0);
    return r->ready;
}

void fd_gl_free(fd_gl_renderer_t *r) {
    if (!r || !r->ready) return;
    glDeleteTextures(FD_TEX_COUNT, r->textures);
    glDeleteBuffers(1, &r->ebo);
    glDeleteBuffers(1, &r->vbo);
    glDeleteVertexArrays(1, &r->vao);
    glDeleteProgram(r->prog);
    memset(r, 0, sizeof(*r));
}

void fd_gl_update_font_tex(fd_gl_renderer_t *r, const uint8_t *rgba, int w, int h) {
    if (!r || !r->ready || !rgba || w <= 0 || h <= 0) return;
    glBindTexture(GL_TEXTURE_2D, r->textures[FD_TEX_FONT]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}

void fd_gl_update_slice_tex(fd_gl_renderer_t *r, const uint8_t *rgba, int w, int h) {
    if (!r || !r->ready || !rgba || w <= 0 || h <= 0) return;
    glBindTexture(GL_TEXTURE_2D, r->textures[FD_TEX_SLICE]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}

void fd_gl_render(fd_gl_renderer_t *r, const fd_ui_t *ui, int vp_w, int vp_h) {
    if (!r || !r->ready || !ui) return;
    glViewport(0, 0, vp_w, vp_h);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.066f, 0.082f, 0.114f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (ui->vert_count <= 0 || ui->index_count <= 0 || ui->cmd_count <= 0) return;
    float log_w = ui->in.width  > 1.0f ? ui->in.width  : (float)vp_w;
    float log_h = ui->in.height > 1.0f ? ui->in.height : (float)vp_h;
    float sx = (float)vp_w / log_w;
    float sy = (float)vp_h / log_h;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST);

    glUseProgram(r->prog);
    glUniform2f(r->u_screen_loc, log_w, log_h);
    glUniform1i(r->u_tex_loc, 0);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(fd_vertex_t) * (size_t)ui->vert_count),
                 ui->verts, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(sizeof(uint16_t) * (size_t)ui->index_count),
                 ui->indices, GL_DYNAMIC_DRAW);

    for (int i = 0; i < ui->cmd_count; i++) {
        const fd_draw_cmd_t *c = &ui->cmds[i];
        int sc_x = (int)(c->clip_x * sx);
        int sc_y = (int)((log_h - (c->clip_y + c->clip_h)) * sy);
        int sc_w = (int)(c->clip_w * sx);
        int sc_h = (int)(c->clip_h * sy);
        if (sc_w <= 0 || sc_h <= 0) continue;
        glScissor(sc_x, sc_y, sc_w, sc_h);
        glBindTexture(GL_TEXTURE_2D, r->textures[c->tex_id]);
        glDrawElements(GL_TRIANGLES, (GLsizei)c->elem_count, GL_UNSIGNED_SHORT,
                       (const void *)(uintptr_t)((size_t)c->elem_offset * sizeof(uint16_t)));
    }

    glDisable(GL_SCISSOR_TEST);
}
