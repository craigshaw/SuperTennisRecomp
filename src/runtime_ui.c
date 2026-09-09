/*
 * runtime_ui.c — in-game runtime settings overlay (recomp-ui RecompRuntimeUi)
 * adapter for the desktop host.
 *
 * Maps the launcher-exposed settings (SuperTennisSettings) onto the shared
 * runtime menu model: sections/items, live-apply callbacks, and input
 * translation. Values persist through SuperTennisSettingsSave into the same
 * config.ini [Settings] the pre-boot launcher reads, so both surfaces agree.
 *
 * Input contract: F1 toggles the menu (gamepad: Select+Start pressed
 * together, so Start alone keeps its in-game meaning). Menu open: Escape /
 * Backspace back out (section -> close), Enter selects, arrows navigate,
 * gamepad D-pad navigates with A=accept/B=back/Start=close. While open every
 * key/button is withheld from the game; closing the menu is the only way back
 * to play (plus the Resume action). The host decides pause policy — see
 * main.c, which stops running frames while the menu is open and drops stale
 * input bits when it closes.
 *
 * This file is SDL3-only in the same sense as the rest of the desktop host's
 * render path (ungated SDL3 calls in main.c); the SDL2 build falls back to
 * the pre-overlay host.
 */
#include <stdlib.h>
#include <string.h>

#include "runtime_ui.h"
#include "config.h"
#include "desktop/sdl_compat.h"
#include "recomp_runtime_ui.h"

/* Same framebuffer size as src/main.c. */
#define SNES_W 256
#define SNES_H 224

/* Per-device Select+Start chord tracking; matches the two gamepad slots the
 * desktop host maintains in src/main.c. */
#define ST_PAD_CHORD_COUNT 2

typedef struct PadChord {
    SDL_JoystickID id;  /* device this state belongs to; -1 = unused */
    int start_down;
    int select_down;
} PadChord;

static PadChord *PadChordFor(PadChord *chords, SDL_JoystickID id) {
    for (int i = 0; i < ST_PAD_CHORD_COUNT; i++)
        if (chords[i].id == id) return &chords[i];
    for (int i = 0; i < ST_PAD_CHORD_COUNT; i++)
        if (chords[i].id == (SDL_JoystickID)-1) {
            chords[i].id = id;
            return &chords[i];
        }
    return &chords[0];  /* all slots full: fold into the first */
}

/* Title-specific semantic keys (the standard RECOMP_RUNTIME_UI_KEY_* values
 * are used where the shared catalog already has a matching setting). */
#define ST_KEY_STRETCH       "graphics.stretch_to_fill"
#define ST_KEY_P1_SOURCE     "input.player1_source"
#define ST_KEY_P1_DEADZONE   "input.player1_deadzone"
#define ST_KEY_P2_SOURCE     "input.player2_source"
#define ST_KEY_P2_DEADZONE   "input.player2_deadzone"
#define ST_KEY_SKIP_LAUNCHER "system.skip_launcher"

struct SuperTennisRuntimeUi {
    SuperTennisSettings *settings;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_AudioStream *audio_stream;
    RecompRuntimeUi *ui;
    /* Physical gamepad state per device (menu closed) so Select+Start — not
     * Start alone — can be the open chord without stealing Start from the
     * game. Kept per pad so either gamepad can open the menu. */
    PadChord pad_chords[ST_PAD_CHORD_COUNT];
};

/* SDL3 renamed the controller EVENT constants (SDL_EVENT_CONTROLLER_* ->
 * SDL_EVENT_GAMEPAD_*); SDL2 only has the old names. The button/axis enums
 * are still provided under old-name aliases by both. Mirrors main.c. */
#if SNESRECOMP_SDL3
#define ST_EVENT_SCANCODE(ev)   ((ev).key.scancode)
#define ST_EVENT_BUTTON_DOWN    SDL_EVENT_GAMEPAD_BUTTON_DOWN
#define ST_EVENT_BUTTON_UP      SDL_EVENT_GAMEPAD_BUTTON_UP
#define ST_EVENT_AXIS_MOTION    SDL_EVENT_GAMEPAD_AXIS_MOTION
#else
#define ST_EVENT_SCANCODE(ev)   ((ev).key.keysym.scancode)
#define ST_EVENT_BUTTON_DOWN    SDL_EVENT_CONTROLLER_BUTTON_DOWN
#define ST_EVENT_BUTTON_UP      SDL_EVENT_CONTROLLER_BUTTON_UP
#define ST_EVENT_AXIS_MOTION    SDL_EVENT_CONTROLLER_AXIS_MOTION
#endif

/* ── menu model ──────────────────────────────────────────────────────────── */

static const char *const kPlayerSourceChoices[] = {"None", "Keyboard", "Gamepad"};
static const int kPlayerSourceValues[] = {0, 1, 2};

static const RecompRuntimeUiItem kItems[] = {
    /* Display */
    {RECOMP_RUNTIME_UI_KEY_FULLSCREEN, "Display", "Fullscreen",
     "Toggle borderless fullscreen.", RECOMP_RUNTIME_UI_BOOL,
     0, 1, 1, NULL, 0, NULL},
    {RECOMP_RUNTIME_UI_KEY_WINDOW_SCALE, "Display", "Window Scale",
     "Window size multiplier; applies immediately.", RECOMP_RUNTIME_UI_INT,
     1, 8, 1, NULL, 0, NULL},
    {ST_KEY_STRETCH, "Display", "Stretch to Fill",
     "Stretch the 4:3 picture to the window instead of integer scaling.",
     RECOMP_RUNTIME_UI_BOOL, 0, 1, 1, NULL, 0, NULL},
    {RECOMP_RUNTIME_UI_KEY_LINEAR_FILTER, "Display", "Linear Filter",
     "Bilinear upscale of the framebuffer.", RECOMP_RUNTIME_UI_BOOL,
     0, 1, 1, NULL, 0, NULL},
    /* Audio */
    {RECOMP_RUNTIME_UI_KEY_VOLUME, "Audio", "Volume",
     "Master volume, percent.", RECOMP_RUNTIME_UI_INT,
     0, 100, 5, NULL, 0, NULL},
    /* Input */
    {ST_KEY_P1_SOURCE, "Input", "Player 1 Source",
     "None, keyboard, or gamepad for player 1.", RECOMP_RUNTIME_UI_CHOICE,
     0, 2, 1, kPlayerSourceChoices, 3, kPlayerSourceValues},
    {ST_KEY_P1_DEADZONE, "Input", "Stick Deadzone",
     "Gamepad stick deadzone for player 1, percent.", RECOMP_RUNTIME_UI_INT,
     0, 100, 5, NULL, 0, NULL},
    {ST_KEY_P2_SOURCE, "Input", "Player 2 Source",
     "None, keyboard, or gamepad for player 2.", RECOMP_RUNTIME_UI_CHOICE,
     0, 2, 1, kPlayerSourceChoices, 3, kPlayerSourceValues},
    {ST_KEY_P2_DEADZONE, "Input", "P2 Stick Deadzone",
     "Gamepad stick deadzone for player 2, percent.", RECOMP_RUNTIME_UI_INT,
     0, 100, 5, NULL, 0, NULL},
    /* System */
    {ST_KEY_SKIP_LAUNCHER, "System", "Skip Launcher",
     "Boot straight into the game next launch from the cached ROM.",
     RECOMP_RUNTIME_UI_BOOL, 0, 1, 1, NULL, 0, NULL},
    {RECOMP_RUNTIME_UI_KEY_RESUME, "System", "Resume",
     "Close the menu and keep playing.", RECOMP_RUNTIME_UI_ACTION,
     0, 0, 0, NULL, 0, NULL},
};

/* ── callbacks (live-apply + persist) ───────────────────────────────────── */

static int GetValue(void *context, const RecompRuntimeUiItem *item,
                    int *value_out) {
  SuperTennisRuntimeUi *rt = (SuperTennisRuntimeUi *)context;
  const SuperTennisSettings *s = rt->settings;
  if (!strcmp(item->key, RECOMP_RUNTIME_UI_KEY_FULLSCREEN))
    *value_out = s->fullscreen;
  else if (!strcmp(item->key, RECOMP_RUNTIME_UI_KEY_WINDOW_SCALE))
    *value_out = s->window_scale;
  else if (!strcmp(item->key, ST_KEY_STRETCH))
    *value_out = s->ignore_aspect;
  else if (!strcmp(item->key, RECOMP_RUNTIME_UI_KEY_LINEAR_FILTER))
    *value_out = s->linear_filter;
  else if (!strcmp(item->key, RECOMP_RUNTIME_UI_KEY_VOLUME))
    *value_out = s->volume;
  else if (!strcmp(item->key, ST_KEY_P1_SOURCE))
    *value_out = s->player_src[0];
  else if (!strcmp(item->key, ST_KEY_P1_DEADZONE))
    *value_out = s->deadzone[0];
  else if (!strcmp(item->key, ST_KEY_P2_SOURCE))
    *value_out = s->player_src[1];
  else if (!strcmp(item->key, ST_KEY_P2_DEADZONE))
    *value_out = s->deadzone[1];
  else if (!strcmp(item->key, ST_KEY_SKIP_LAUNCHER))
    *value_out = s->skip_launcher;
  else
    return 0;
  return 1;
}

static int SetValue(void *context, const RecompRuntimeUiItem *item,
                    int value) {
  SuperTennisRuntimeUi *rt = (SuperTennisRuntimeUi *)context;
  SuperTennisSettings *s = rt->settings;
  if (!strcmp(item->key, RECOMP_RUNTIME_UI_KEY_FULLSCREEN)) {
    s->fullscreen = value != 0;
    snesrecomp_sdl_set_fullscreen(rt->window, s->fullscreen);
  } else if (!strcmp(item->key, RECOMP_RUNTIME_UI_KEY_WINDOW_SCALE)) {
    s->window_scale = value < 1 ? 1 : value;
    SDL_SetWindowSize(rt->window, SNES_W * s->window_scale,
                      SNES_H * s->window_scale);
  } else if (!strcmp(item->key, ST_KEY_STRETCH)) {
    s->ignore_aspect = value != 0;
    SuperTennisRuntimeUiReapplyLogicalPresentation(rt->renderer, s);
  } else if (!strcmp(item->key, RECOMP_RUNTIME_UI_KEY_LINEAR_FILTER)) {
    s->linear_filter = value != 0;
    snesrecomp_sdl_set_texture_linear(rt->texture, s->linear_filter);
  } else if (!strcmp(item->key, RECOMP_RUNTIME_UI_KEY_VOLUME)) {
    s->volume = value < 0 ? 0 : (value > 100 ? 100 : value);
    if (rt->audio_stream)
      SDL_SetAudioStreamGain(rt->audio_stream, (float)s->volume / 100.0f);
  } else if (!strcmp(item->key, ST_KEY_P1_SOURCE)) {
    /* Applied live by main.c, which syncs the gamepads each loop iteration. */
    s->player_src[0] = value;
  } else if (!strcmp(item->key, ST_KEY_P1_DEADZONE)) {
    /* Read live by ApplyStickToDpad in main.c. */
    s->deadzone[0] = value < 0 ? 0 : (value > 100 ? 100 : value);
  } else if (!strcmp(item->key, ST_KEY_P2_SOURCE)) {
    /* Applied live by main.c, which syncs the gamepads each loop iteration. */
    s->player_src[1] = value;
  } else if (!strcmp(item->key, ST_KEY_P2_DEADZONE)) {
    /* Read live by ApplyStickToDpad in main.c. */
    s->deadzone[1] = value < 0 ? 0 : (value > 100 ? 100 : value);
  } else if (!strcmp(item->key, ST_KEY_SKIP_LAUNCHER)) {
    s->skip_launcher = value != 0;
  } else {
    return 0;
  }
  return 1;
}

static int RunAction(void *context, const RecompRuntimeUiItem *item) {
  SuperTennisRuntimeUi *rt = (SuperTennisRuntimeUi *)context;
  if (!strcmp(item->key, RECOMP_RUNTIME_UI_KEY_RESUME)) {
    recomp_runtime_ui_close(rt->ui);
    return 1;
  }
  return 0;
}

static int IsEnabled(void *context, const RecompRuntimeUiItem *item) {
  SuperTennisRuntimeUi *rt = (SuperTennisRuntimeUi *)context;
  if (!strcmp(item->key, ST_KEY_P1_DEADZONE))
    return rt->settings->player_src[0] == 2;
  if (!strcmp(item->key, ST_KEY_P2_DEADZONE))
    return rt->settings->player_src[1] == 2;
  return 1;
}

static void Save(void *context) {
  SuperTennisRuntimeUi *rt = (SuperTennisRuntimeUi *)context;
  /* Same file the pre-boot launcher and its hotkey editor write; the surgical
   * in-place update preserves the launcher's [KeyMap] section. */
  SuperTennisSettingsSave("config.ini", rt->settings);
}

static void VisibilityChanged(void *context, int open) {
  SuperTennisRuntimeUi *rt = (SuperTennisRuntimeUi *)context;
  /* The host stops running frames while the menu is open, so nothing feeds
   * the audio stream; pause the device to stop the pull (clean silence
   * instead of underrun churn) and resume on close. */
  if (rt->audio_stream) {
    if (open)
      SDL_PauseAudioStreamDevice(rt->audio_stream);
    else
      SDL_ResumeAudioStreamDevice(rt->audio_stream);
  }
}

/* ── public API ─────────────────────────────────────────────────────────── */

SuperTennisRuntimeUi *SuperTennisRuntimeUiCreate(
    SuperTennisSettings *settings, SDL_Window *window, SDL_Renderer *renderer,
    SDL_Texture *texture, SDL_AudioStream *audio_stream) {
  SuperTennisRuntimeUi *rt = (SuperTennisRuntimeUi *)calloc(1, sizeof(*rt));
  if (!rt) return NULL;
  rt->settings = settings;
  rt->window = window;
  rt->renderer = renderer;
  rt->texture = texture;
  rt->audio_stream = audio_stream;

  for (int i = 0; i < ST_PAD_CHORD_COUNT; i++)
    rt->pad_chords[i].id = (SDL_JoystickID)-1;

  RecompRuntimeUiConfig config;
  memset(&config, 0, sizeof(config));
  config.title = "Super Tennis";
  config.subtitle = "Runtime Settings";
  config.items = kItems;
  config.item_count = sizeof(kItems) / sizeof(kItems[0]);
  config.callbacks.context = rt;
  config.callbacks.get_value = GetValue;
  config.callbacks.set_value = SetValue;
  config.callbacks.run_action = RunAction;
  config.callbacks.is_enabled = IsEnabled;
  config.callbacks.save = Save;
  config.callbacks.visibility_changed = VisibilityChanged;
  config.theme = "snes";
  config.accept_label = "Enter";
  config.back_label = "Backspace";

  rt->ui = recomp_runtime_ui_create(&config);
  if (!rt->ui) {
    free(rt);
    return NULL;
  }
  return rt;
}

void SuperTennisRuntimeUiDestroy(SuperTennisRuntimeUi *rt) {
  if (!rt) return;
  if (rt->ui) recomp_runtime_ui_destroy(rt->ui);
  free(rt);
}

int SuperTennisRuntimeUiIsOpen(const SuperTennisRuntimeUi *rt) {
  return rt && rt->ui && recomp_runtime_ui_is_open(rt->ui);
}

void SuperTennisRuntimeUiOpen(SuperTennisRuntimeUi *rt) {
  if (rt && rt->ui) recomp_runtime_ui_open(rt->ui);
}

int SuperTennisRuntimeUiHandleEvent(SuperTennisRuntimeUi *rt,
                                    const SDL_Event *event) {
  if (!rt || !rt->ui) return 0;
  RecompRuntimeUi *ui = rt->ui;
  const int open = recomp_runtime_ui_is_open(ui);
  RecompRuntimeUiInput in = (RecompRuntimeUiInput)-1;
  int pressed = 0;
  int repeat = 0;

  switch (event->type) {
  case SDL_EVENT_KEY_DOWN:
  case SDL_EVENT_KEY_UP:
    pressed = event->type == SDL_EVENT_KEY_DOWN;
    repeat = pressed && event->key.repeat;
    switch (ST_EVENT_SCANCODE(*event)) {
    case SDL_SCANCODE_F1: in = RECOMP_RUNTIME_UI_INPUT_TOGGLE; break;
    case SDL_SCANCODE_ESCAPE: in = RECOMP_RUNTIME_UI_INPUT_BACK; break;
    case SDL_SCANCODE_RETURN: in = RECOMP_RUNTIME_UI_INPUT_ACCEPT; break;
    case SDL_SCANCODE_BACKSPACE: in = RECOMP_RUNTIME_UI_INPUT_BACK; break;
    case SDL_SCANCODE_UP: in = RECOMP_RUNTIME_UI_INPUT_UP; break;
    case SDL_SCANCODE_DOWN: in = RECOMP_RUNTIME_UI_INPUT_DOWN; break;
    case SDL_SCANCODE_LEFT: in = RECOMP_RUNTIME_UI_INPUT_LEFT; break;
    case SDL_SCANCODE_RIGHT: in = RECOMP_RUNTIME_UI_INPUT_RIGHT; break;
    default:
      /* Unknown key: withheld while the menu is open; the game sees it when
       * the menu is closed (the game never binds F1, so no clash). */
      return open ? 1 : 0;
    }
    break;
  case ST_EVENT_BUTTON_DOWN:
  case ST_EVENT_BUTTON_UP: {
    pressed = event->type == ST_EVENT_BUTTON_DOWN;
    const int button = SNESRECOMP_SDL_EVENT_BUTTON(*event);
    /* Track the physical pad state per device regardless of the menu, so the
     * closed menu can detect the Select+Start open chord below on whichever
     * pad the player uses. */
    PadChord *chord =
        PadChordFor(rt->pad_chords,
                    SNESRECOMP_SDL_EVENT_BUTTON_DEVICE(*event));
    if (button == SDL_CONTROLLER_BUTTON_START)
      chord->start_down = pressed;
    else if (button == SDL_CONTROLLER_BUTTON_BACK) /* the Select button */
      chord->select_down = pressed;
    if (!open) {
      /* Menu closed: only the Select+Start chord opens it; every other pad
       * input — including Start alone, the game's pause/menu button — is the
       * game's. */
      if (pressed && !repeat &&
          (button == SDL_CONTROLLER_BUTTON_START ||
           button == SDL_CONTROLLER_BUTTON_BACK) &&
          chord->select_down && chord->start_down) {
        return recomp_runtime_ui_handle_input(
                   ui, RECOMP_RUNTIME_UI_INPUT_TOGGLE, 1, 0) != 0;
      }
      return 0;
    }
    switch (button) {
    case SDL_CONTROLLER_BUTTON_START: in = RECOMP_RUNTIME_UI_INPUT_TOGGLE; break;
    case SDL_CONTROLLER_BUTTON_A: in = RECOMP_RUNTIME_UI_INPUT_ACCEPT; break;
    case SDL_CONTROLLER_BUTTON_B: in = RECOMP_RUNTIME_UI_INPUT_BACK; break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: in = RECOMP_RUNTIME_UI_INPUT_UP; break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: in = RECOMP_RUNTIME_UI_INPUT_DOWN; break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: in = RECOMP_RUNTIME_UI_INPUT_LEFT; break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: in = RECOMP_RUNTIME_UI_INPUT_RIGHT; break;
    default:
      /* Any other pad input is withheld while the menu is open. */
      return 1;
    }
    break;
  }
  case ST_EVENT_AXIS_MOTION:
    /* The D-pad drives the menu; consume stick motion while open so it does
     * not leak into the (paused) game input state. */
    return open ? 1 : 0;
  default:
    return 0;
  }

  /* Route to the shared menu model. Its return value IS the consume verdict:
   * a closed menu consumes only the F1 toggle (opening it), so the game gets
   * every other key — Enter/arrows included; an open menu consumes all. */
  if ((int)in >= 0)
    return recomp_runtime_ui_handle_input(ui, in, pressed, repeat) != 0;
  return open ? 1 : 0;
}

void SuperTennisRuntimeUiRestoreRendererState(SuperTennisRuntimeUi *rt,
                                              SDL_Renderer *renderer) {
  if (!rt || !renderer) return;
  /* The ImGui renderer backend restores the viewport to the concrete rect it
   * captured while the logical presentation was temporarily disabled. If that
   * rect survived, re-enabling the 256x224 presentation would reinterpret it
   * in logical coordinates (viewport coords are logical-space in SDL3), giving
   * a many-times-larger viewport anchored top-left — the picture "zooms into
   * the top-left corner". Reset the viewport to the scale-proof full-target
   * state first, then re-enable the logical presentation. Draw commands
   * already queued (game + overlay) keep the scale they were queued with;
   * only the state for the next frame changes here. */
  SDL_SetRenderViewport(renderer, NULL);
  SuperTennisRuntimeUiReapplyLogicalPresentation(renderer, rt->settings);
}

void SuperTennisRuntimeUiReapplyLogicalPresentation(
    SDL_Renderer *renderer, const SuperTennisSettings *settings) {
  SDL_SetRenderLogicalPresentation(
      renderer, SNES_W, SNES_H,
      settings->ignore_aspect ? SDL_LOGICAL_PRESENTATION_STRETCH
                              : SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
}

RecompRuntimeUi *SuperTennisRuntimeUiCore(const SuperTennisRuntimeUi *rt) {
  return rt ? rt->ui : NULL;
}