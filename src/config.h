#pragma once

#include <stdint.h>

/* Desktop host settings for the shared recomp-ui pre-boot launcher.
 *
 * Exactly the launcher-exposed fields this host can honor. The pre-boot
 * window seeds from this struct and the user's choices map back into it on
 * Play; the host then applies them (window size, fullscreen, filtering,
 * audio, input source) and persists them to config.ini [Settings] (see
 * config.c). Defaults reproduce the pre-launcher host behavior exactly:
 * 256x224 fixed window, integer scale, 48 kHz stereo audio, keyboard-only.
 */
typedef struct SuperTennisSettings {
    int output_method;   /* 0 SDL (the only backend this host implements) */
    int window_scale;    /* 1..N initial window size multiplier */
    int fullscreen;      /* 0 off, 1 borderless, 2 exclusive */
    int ignore_aspect;   /* bool: stretch to fill instead of integer scale */
    int linear_filter;   /* bool: bilinear upscale */
    int enable_audio;    /* bool */
    int audio_freq;      /* Hz */
    int volume;          /* 0..100 */
    int player_src[2];   /* 0 none, 1 keyboard, 2 gamepad */
    int deadzone[2];     /* 0..100, gamepad stick deadzone */
    int skip_launcher;   /* bool: boot straight to the game next run */
} SuperTennisSettings;

static inline void SuperTennisSettingsInitDefault(
    SuperTennisSettings *s) {
    s->output_method = 0;
    s->window_scale = 1;
    s->fullscreen = 0;
    s->ignore_aspect = 0;
    s->linear_filter = 0;
    s->enable_audio = 1;
    s->audio_freq = 48000;
    s->volume = 100;
    s->player_src[0] = 1;   /* P1 keyboard, matching the pre-launcher host */
    s->player_src[1] = 0;
    s->deadzone[0] = 50;
    s->deadzone[1] = 50;
    s->skip_launcher = 0;
}

/* Load [Settings] from `path` over the given defaults. Missing file/keys
 * leave the struct untouched. */
void SuperTennisSettingsLoad(const char *path, SuperTennisSettings *s);

/* Persist the struct to `path` as a surgical in-place [Settings] update that
 * preserves all other sections and comments verbatim. */
void SuperTennisSettingsSave(const char *path, const SuperTennisSettings *s);

/* Fill `map` (SDL_NUM_SCANCODES entries, scancode -> runner input bit) with
 * the host's default keyboard layout. */
void SuperTennisKeyBindsDefaults(uint32_t *map, int n);

/* Seed `path` (keybinds.ini) with the host's default layout when missing. */
void SuperTennisKeyBindsWriteDefaults(const char *path);

/* Apply [player1]/[player2] of `path` over `maps[2]` (restart-to-apply,
 * like the launcher's own keybinds.ini contract). */
void SuperTennisKeyBindsLoad(const char *path, uint32_t *maps[2], int n);
