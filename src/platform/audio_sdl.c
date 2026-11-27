#include "audio_sdl.h"

#include <stdlib.h>
#include <string.h>

static bool load_clip(const fd_vfs_t *vfs, const char *rel_path, fd_wav_clip_t *out) {
    uint8_t *raw = NULL;
    size_t sz = 0;
    if (!fd_vfs_read(vfs, rel_path, &raw, &sz)) return false;
    SDL_IOStream *io = SDL_IOFromConstMem(raw, sz);
    if (!io) { free(raw); return false; }
    bool ok = SDL_LoadWAV_IO(io, true, &out->spec, &out->buf, &out->len);
    free(raw);
    return ok;
}

bool fd_audio_init(fd_audio_t *aud, const fd_vfs_t *vfs) {
    if (!aud || !vfs) return false;
    memset(aud, 0, sizeof(*aud));

    if (!load_clip(vfs, "sounds/sizzle_loop.wav", &aud->sizzle) ||
        !load_clip(vfs, "sounds/boil_loop.wav",   &aud->boil) ||
        !load_clip(vfs, "sounds/sear_drop.wav",   &aud->sear_drop) ||
        !load_clip(vfs, "sounds/chop.wav",        &aud->chop) ||
        !load_clip(vfs, "sounds/timer.wav",       &aud->timer)) {
        return false;
    }

    aud->dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &aud->sizzle.spec);
    if (!aud->dev) {
        /* Headless CI or muted browser: degrade gracefully without failing app startup */
        return true;
    }

    aud->sizzle_stream = SDL_CreateAudioStream(&aud->sizzle.spec, &aud->sizzle.spec);
    aud->boil_stream   = SDL_CreateAudioStream(&aud->boil.spec,   &aud->sizzle.spec);
    aud->cue_stream    = SDL_CreateAudioStream(&aud->chop.spec,   &aud->sizzle.spec);

    if (aud->sizzle_stream) SDL_BindAudioStream(aud->dev, aud->sizzle_stream);
    if (aud->boil_stream)   SDL_BindAudioStream(aud->dev, aud->boil_stream);
    if (aud->cue_stream)    SDL_BindAudioStream(aud->dev, aud->cue_stream);
    SDL_ResumeAudioDevice(aud->dev);
    aud->enabled = true;
    return true;
}

void fd_audio_free(fd_audio_t *aud) {
    if (!aud) return;
    if (aud->sizzle_stream) SDL_DestroyAudioStream(aud->sizzle_stream);
    if (aud->boil_stream)   SDL_DestroyAudioStream(aud->boil_stream);
    if (aud->cue_stream)    SDL_DestroyAudioStream(aud->cue_stream);
    if (aud->dev)           SDL_CloseAudioDevice(aud->dev);

    SDL_free(aud->sizzle.buf);
    SDL_free(aud->boil.buf);
    SDL_free(aud->sear_drop.buf);
    SDL_free(aud->chop.buf);
    SDL_free(aud->timer.buf);
    memset(aud, 0, sizeof(*aud));
}

void fd_audio_update(fd_audio_t *aud, float sizzle_flux, float boil_flux, fd_sound_cue_t cue) {
    if (!aud || !aud->enabled) return;

    if (aud->sizzle_stream) {
        SDL_SetAudioStreamGain(aud->sizzle_stream, sizzle_flux * 0.45f);
        if (sizzle_flux > 0.02f && SDL_GetAudioStreamQueued(aud->sizzle_stream) < (int)aud->sizzle.len) {
            SDL_PutAudioStreamData(aud->sizzle_stream, aud->sizzle.buf, (int)aud->sizzle.len);
        }
    }
    if (aud->boil_stream) {
        SDL_SetAudioStreamGain(aud->boil_stream, boil_flux * 0.50f);
        if (boil_flux > 0.02f && SDL_GetAudioStreamQueued(aud->boil_stream) < (int)aud->boil.len) {
            SDL_PutAudioStreamData(aud->boil_stream, aud->boil.buf, (int)aud->boil.len);
        }
    }
    if (aud->cue_stream && cue != FD_CUE_NONE) {
        const fd_wav_clip_t *c = NULL;
        if (cue == FD_CUE_SEAR_DROP) c = &aud->sear_drop;
        else if (cue == FD_CUE_CHOP) c = &aud->chop;
        else if (cue == FD_CUE_TIMER) c = &aud->timer;
        if (c && c->buf) {
            SDL_SetAudioStreamGain(aud->cue_stream, 0.55f);
            SDL_PutAudioStreamData(aud->cue_stream, c->buf, (int)c->len);
        }
    }
}
