#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "../core/fd_app.h"
#include "audio_sdl.h"
#include "gl_renderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct {
    SDL_Window       *window;
    SDL_GLContext     gl_ctx;
    fd_app_t          app;
    fd_gl_renderer_t  gl;
    fd_audio_t        audio;
    fd_input_t        input;
    uint64_t          last_ticks_ns;
    bool              booted;
    bool              prev_mod_boost;
} fd_host_t;

static fd_host_t *g_host = NULL;

#ifdef __EMSCRIPTEN__
static char g_state_json[640];

EM_JS(void, js_start_asset_fetch, (void), {
    (async function() {
        try {
            const resp = await fetch('./assets/manifest.json', { cache: 'no-cache' });
            const text = await resp.text();
            const paths = JSON.parse(text);
            const enc = new TextEncoder();
            {
                const mb = enc.encode(text);
                const ptr = Module._malloc(mb.length);
                Module.HEAPU8.set(mb, ptr);
                const sp = Module.stringToNewUTF8('manifest.json');
                Module._fd_web_push_asset(sp, ptr, mb.length, 0);
                Module._free(sp);
                Module._free(ptr);
            }
            await Promise.all(paths.map(async (p) => {
                const r = await fetch('./assets/' + p, { cache: 'no-cache' });
                if (!r.ok) return;
                const buf = new Uint8Array(await r.arrayBuffer());
                const ptr = Module._malloc(buf.length);
                Module.HEAPU8.set(buf, ptr);
                const sp = Module.stringToNewUTF8(p);
                Module._fd_web_push_asset(sp, ptr, buf.length, 0);
                Module._free(sp);
                Module._free(ptr);
            }));
            /* Overlay persisted localStorage Lua / locale overrides */
            for (let i = 0; i < window.localStorage.length; i++) {
                const k = window.localStorage.key(i);
                if (k && k.startsWith('fatduck_mod:')) {
                    const rel = k.substring('fatduck_mod:'.length);
                    const val = window.localStorage.getItem(k) || "";
                    const bytes = enc.encode(val);
                    const ptr = Module._malloc(bytes.length);
                    Module.HEAPU8.set(bytes, ptr);
                    const sp = Module.stringToNewUTF8(rel);
                    Module._fd_web_push_asset(sp, ptr, bytes.length, 1);
                    Module._free(sp);
                    Module._free(ptr);
                }
            }
            const isMobile = (navigator.maxTouchPoints > 0) || (window.innerWidth < 820);
            const savedLang = window.localStorage.getItem('fatduck_lang') || navigator.language || 'en';
            const lp = Module.stringToNewUTF8(savedLang);
            Module._fd_web_finish_boot(isMobile ? 1 : 0, lp);
            Module._free(lp);
        } catch (err) {
            console.error('FatDuck web asset bootstrap failed:', err);
        }
    })();
});

EM_JS(void, js_sync_local_mod, (const char *rel_path, const char *content, int active), {
    try {
        const key = 'fatduck_mod:' + UTF8ToString(rel_path);
        if (active) window.localStorage.setItem(key, UTF8ToString(content));
        else window.localStorage.removeItem(key);
    } catch (e) {}
});

EM_JS(void, js_save_lang, (const char *code), {
    try { window.localStorage.setItem('fatduck_lang', UTF8ToString(code)); } catch (e) {}
});

EMSCRIPTEN_KEEPALIVE void fd_web_push_asset(const char *rel_path, const uint8_t *data, int len, int is_override) {
    if (!g_host || !rel_path || len < 0) return;
    fd_vfs_put(&g_host->app.vfs, rel_path, data, (size_t)len, is_override != 0);
}

EMSCRIPTEN_KEEPALIVE void fd_web_finish_boot(int is_mobile_touch, const char *lang_code) {
    if (!g_host || g_host->booted) return;
    fd_app_t *app = &g_host->app;
    if (!fd_lua_init(&app->lua, &app->vfs, &app->db) ||
        !fd_lua_exec_file(&app->lua, "scripts/init.lua", 2000000)) {
        SDL_Log("FatDuck Lua boot failed: %s", app->lua.last_error);
        return;
    }
    fd_i18n_init(&app->i18n, &app->lua, lang_code ? lang_code : "en");
    if (!fd_ui_init(&app->ui, &app->vfs, &app->i18n)) {
        SDL_Log("FatDuck Nuklear UI init failed");
        return;
    }
    app->time_scale       = 5.0f;
    app->show_welcome     = true;
    app->pending_lang_idx = -1;
    app->slice_plane      = FD_SLICE_XZ;
    app->slice_norm   = 0.5f;
    app->overlay_mode = FD_OVERLAY_TEMP;
    app->touch_mode   = (is_mobile_touch != 0);
    fd_constraints_init_default(&app->constraints);
    if (app->db.recipe_count > 0) fd_app_load_recipe_index(app, 0);

    fd_gl_init(&g_host->gl, &app->vfs);
    if (app->ui.atlas_dirty && app->ui.atlas_rgba) {
        fd_gl_update_font_tex(&g_host->gl, app->ui.atlas_rgba, app->ui.atlas_w, app->ui.atlas_h);
        app->ui.atlas_dirty = false;
    }
    fd_audio_init(&g_host->audio, &app->vfs);
    if (app->slice_dirty) {
        fd_gl_update_slice_tex(&g_host->gl, app->slice_rgba, app->slice_w, app->slice_h);
        app->slice_dirty = false;
    }
    g_host->booted = true;
}

EMSCRIPTEN_KEEPALIVE void fd_web_set_lang(const char *code) {
    if (!g_host || !g_host->booted || !code) return;
    fd_app_set_language(&g_host->app, fd_i18n_find_lang(code));
}

EMSCRIPTEN_KEEPALIVE const char *fd_web_get_state_json(void) {
    if (!g_host) return "{}";
    const fd_app_t *a = &g_host->app;
    const fd_sim_state_t *sv = fd_app_vessel_const(a);
    const fd_lang_info_t *li = fd_i18n_lang_info(a->i18n.active_lang);
    snprintf(g_state_json, sizeof(g_state_json),
             "{\"booted\":%s,\"ui\":\"nuklear\",\"touch_mode\":%s,\"tab\":%d,"
             "\"lang\":\"%s\",\"lang_count\":%d,\"help_open\":%s,\"recipe_card_open\":%s,"
             "\"tutorial_open\":%s,\"tutorial_step\":%d,\"vert_count\":%d,"
             "\"ingredients\":%d,\"techniques\":%d,\"recipes\":%d,\"mods\":%d,"
             "\"vessels\":%d,\"active_vessel\":%d,\"step\":%d,"
             "\"score\":%.2f,\"core_c\":%.2f}",
             g_host->booted ? "true" : "false",
             a->touch_mode ? "true" : "false",
             (int)a->active_tab,
             li->code,
             FD_LANG_COUNT,
             a->help_open ? "true" : "false",
             a->recipe_card_open ? "true" : "false",
             a->tutorial_open ? "true" : "false",
             a->tutorial_step,
             a->ui.vert_count,
             a->db.ingredient_count,
             a->db.technique_count,
             a->db.recipe_count,
             a->lua.mod_count,
             a->kitchen.vessel_count,
             a->kitchen.active_vessel,
             a->kitchen.current_step,
             sv->metrics.overall_score,
             sv->metrics.core_temp_c);
    return g_state_json;
}
#endif

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    fd_host_t *h = (fd_host_t *)calloc(1, sizeof(fd_host_t));
    if (!h) return SDL_APP_FAILURE;
    *appstate = h;
    g_host = h;

    SDL_WindowFlags wflags = SDL_WINDOW_OPENGL;
#ifndef __EMSCRIPTEN__
    wflags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
    h->window = SDL_CreateWindow("FatDuck — 3D First-Principles Culinary Lab", 1240, 760, wflags);
    if (!h->window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    h->gl_ctx = SDL_GL_CreateContext(h->window);
    if (!h->gl_ctx) {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_GL_MakeCurrent(h->window, h->gl_ctx);
    SDL_GL_SetSwapInterval(1);

    h->last_ticks_ns = SDL_GetTicksNS();

#ifdef __EMSCRIPTEN__
    fd_vfs_init(&h->app.vfs, "assets", NULL);
    js_start_asset_fetch();
#else
    const char *base = SDL_GetBasePath();
    const char *env_lang = getenv("FATDUCK_LANG");
    if (!fd_app_init_with_lang(&h->app, NULL, base, env_lang ? env_lang : "en")) {
        SDL_Log("fd_app_init failed: %s", h->app.status_msg);
        return SDL_APP_FAILURE;
    }
    int num_touch = 0;
    SDL_TouchID *tdevs = SDL_GetTouchDevices(&num_touch);
    if (tdevs) {
        if (num_touch > 0) h->app.touch_mode = true;
        SDL_free(tdevs);
    }
    if (!fd_gl_init(&h->gl, &h->app.vfs)) {
        SDL_Log("fd_gl_init failed");
        return SDL_APP_FAILURE;
    }
    if (h->app.ui.atlas_dirty && h->app.ui.atlas_rgba) {
        fd_gl_update_font_tex(&h->gl, h->app.ui.atlas_rgba, h->app.ui.atlas_w, h->app.ui.atlas_h);
        h->app.ui.atlas_dirty = false;
    }
    fd_audio_init(&h->audio, &h->app.vfs);
    h->booted = true;
#endif
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *ev) {
    fd_host_t *h = (fd_host_t *)appstate;
    if (!h) return SDL_APP_CONTINUE;

    int win_w = 1240, win_h = 760;
    SDL_GetWindowSize(h->window, &win_w, &win_h);

    switch (ev->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_MOUSE_MOTION:
            h->input.pointer_x = ev->motion.x;
            h->input.pointer_y = ev->motion.y;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            h->input.scroll_y += ev->wheel.y;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (ev->button.button == SDL_BUTTON_LEFT) {
                h->input.pointer_x       = ev->button.x;
                h->input.pointer_y       = ev->button.y;
                h->input.pointer_down    = true;
                h->input.pointer_pressed = true;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (ev->button.button == SDL_BUTTON_LEFT) {
                h->input.pointer_x        = ev->button.x;
                h->input.pointer_y        = ev->button.y;
                h->input.pointer_down     = false;
                h->input.pointer_released = true;
            }
            break;
        case SDL_EVENT_FINGER_DOWN:
            h->input.is_touch_event  = true;
            h->input.pointer_x       = ev->tfinger.x * (float)win_w;
            h->input.pointer_y       = ev->tfinger.y * (float)win_h;
            h->input.pointer_down    = true;
            h->input.pointer_pressed = true;
            if (h->audio.dev) SDL_ResumeAudioDevice(h->audio.dev);
            break;
        case SDL_EVENT_FINGER_MOTION:
            h->input.is_touch_event = true;
            h->input.pointer_x      = ev->tfinger.x * (float)win_w;
            h->input.pointer_y      = ev->tfinger.y * (float)win_h;
            h->input.scroll_y      += ev->tfinger.dy * 10.0f;
            break;
        case SDL_EVENT_FINGER_UP:
            h->input.is_touch_event   = true;
            h->input.pointer_x        = ev->tfinger.x * (float)win_w;
            h->input.pointer_y        = ev->tfinger.y * (float)win_h;
            h->input.pointer_down     = false;
            h->input.pointer_released = true;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (ev->key.key == SDLK_ESCAPE) {
                if (h->app.recipe_card_open) { h->app.recipe_card_open = false; break; }
                if (h->app.help_open) { h->app.help_open = false; break; }
                if (h->app.tutorial_open) { h->app.tutorial_open = false; break; }
                return SDL_APP_SUCCESS;
            }
            if (ev->key.key == SDLK_F1)     h->input.key_help   = true;
            if (ev->key.key == SDLK_SPACE)  h->input.key_space  = true;
            if (ev->key.key == SDLK_F)      h->input.key_flip   = true;
            if (ev->key.key == SDLK_F5)     h->input.key_reload = true;
            if (ev->key.key == SDLK_X || ev->key.key == SDLK_TAB) h->input.key_plane = true;
            if (ev->key.key >= SDLK_1 && ev->key.key <= SDLK_5) {
                h->input.speed_key = (int)(ev->key.key - SDLK_1) + 1;
            }
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    fd_host_t *h = (fd_host_t *)appstate;
    if (!h) return SDL_APP_FAILURE;

    uint64_t now = SDL_GetTicksNS();
    float dt = (float)((double)(now - h->last_ticks_ns) * 1e-9);
    h->last_ticks_ns = now;

    int log_w = 1240, log_h = 760, pix_w = 1240, pix_h = 760;
    SDL_GetWindowSize(h->window, &log_w, &log_h);
    SDL_GetWindowSizeInPixels(h->window, &pix_w, &pix_h);
    h->input.width     = (float)(log_w > 0 ? log_w : 1240);
    h->input.height    = (float)(log_h > 0 ? log_h : 760);
    h->input.dpi_scale = (log_w > 0) ? ((float)pix_w / (float)log_w) : 1.0f;

    if (h->booted) {
        fd_app_update_and_render(&h->app, &h->input, dt);

        if (h->app.ui.atlas_dirty && h->app.ui.atlas_rgba) {
            fd_gl_update_font_tex(&h->gl, h->app.ui.atlas_rgba, h->app.ui.atlas_w, h->app.ui.atlas_h);
            h->app.ui.atlas_dirty = false;
        }
        if (h->app.slice_dirty) {
            fd_gl_update_slice_tex(&h->gl, h->app.slice_rgba, h->app.slice_w, h->app.slice_h);
            h->app.slice_dirty = false;
        }
        if (h->app.pending_url[0] != '\0') {
            SDL_OpenURL(h->app.pending_url);
            h->app.pending_url[0] = '\0';
        }
        fd_gl_render(&h->gl, &h->app.ui, pix_w, pix_h);
        fd_audio_update(&h->audio, h->app.kitchen.sizzle_flux, h->app.kitchen.boil_flux, h->app.pending_cue);

#ifdef __EMSCRIPTEN__
        if (h->app.lang_changed) {
            h->app.lang_changed = false;
            js_save_lang(fd_i18n_lang_info(h->app.i18n.active_lang)->code);
        }
        if (h->app.mod_boost_enabled != h->prev_mod_boost) {
            h->prev_mod_boost = h->app.mod_boost_enabled;
            uint8_t *mbuf = NULL;
            size_t msz = 0;
            if (h->app.mod_boost_enabled && fd_vfs_read(&h->app.vfs, "scripts/mods/example_mod.lua", &mbuf, &msz)) {
                js_sync_local_mod("scripts/mods/example_mod.lua", (const char *)mbuf, 1);
                free(mbuf);
            } else {
                js_sync_local_mod("scripts/mods/example_mod.lua", "", 0);
            }
        }
#endif
    }

    /* Reset single-frame edge flags */
    h->input.scroll_y         = 0.0f;
    h->input.pointer_pressed  = false;
    h->input.pointer_released = false;
    h->input.is_touch_event   = false;
    h->input.key_space        = false;
    h->input.key_flip         = false;
    h->input.key_reload       = false;
    h->input.key_plane        = false;
    h->input.key_help         = false;
    h->input.speed_key        = 0;

    SDL_GL_SwapWindow(h->window);
    const char *max_frames_env = getenv("FATDUCK_AUTOTEST_FRAMES");
    if (max_frames_env && max_frames_env[0] != '\0') {
        static int s_frames = 0;
        if (++s_frames >= atoi(max_frames_env)) return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)result;
    fd_host_t *h = (fd_host_t *)appstate;
    if (!h) return;
    fd_audio_free(&h->audio);
    fd_gl_free(&h->gl);
    fd_app_free(&h->app);
    if (h->gl_ctx) SDL_GL_DestroyContext(h->gl_ctx);
    if (h->window) SDL_DestroyWindow(h->window);
    free(h);
    g_host = NULL;
}
