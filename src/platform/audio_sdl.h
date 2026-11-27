#ifndef FD_AUDIO_SDL_H
#define FD_AUDIO_SDL_H

#include "../core/fd_app.h"
#include <SDL3/SDL.h>
#include <stdbool.h>

typedef struct {
    SDL_AudioSpec spec;
    uint8_t      *buf;
    uint32_t      len;
} fd_wav_clip_t;

typedef struct {
    SDL_AudioDeviceID dev;
    SDL_AudioStream  *sizzle_stream;
    SDL_AudioStream  *boil_stream;
    SDL_AudioStream  *cue_stream;

    fd_wav_clip_t     sizzle;
    fd_wav_clip_t     boil;
    fd_wav_clip_t     sear_drop;
    fd_wav_clip_t     chop;
    fd_wav_clip_t     timer;
    bool              enabled;
} fd_audio_t;

bool fd_audio_init(fd_audio_t *aud, const fd_vfs_t *vfs);
void fd_audio_free(fd_audio_t *aud);
void fd_audio_update(fd_audio_t *aud, float sizzle_flux, float boil_flux, fd_sound_cue_t cue);

#endif /* FD_AUDIO_SDL_H */
