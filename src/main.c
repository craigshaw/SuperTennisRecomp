/*
 * main.c — minimal SDL3 desktop host for the Super Tennis recomp, with the
 * shared recomp-ui pre-boot launcher (RECOMP_LAUNCHER). Responsibilities:
 *
 *   - ROM discovery: positional path -> rom.cfg cache -> native file picker,
 *     via snesrecomp_launcher_resolve_rom_sha256(); when the launcher window
 *     runs, recomp_launcher_run_window() owns the same resolution and caches
 *     the picked ROM back to rom.cfg,
 *   - seed/collect the launcher settings into a SuperTennisSettings struct
 *     and apply them to the window/renderer/audio/input,
 *   - verify and load the private reference ROM (SHA-256 identity),
 *   - RtlRegisterGame + SnesInit (SRAM 2 KB mapped from the cart header),
 *   - SDL3 window/renderer/streaming texture at 256x224,
 *   - SDL3 audio stream feeding RtlRenderAudio (SPC at 32040 Hz converted
 *     onto the opened device rate via RtlSetAudioOutputRate),
 *   - keyboard/gamepad -> 12-bit runner input word -> RtlRunFrame(),
 *   - draw: g_rtl_game_info->draw_ppu_frame() renders the PPU field into
 *     the pixel buffer (PpuBeginDrawing target), then blit + present,
 *   - ~60 fps pacing.
 *
 * The launcher window never runs for --replay runs, SNESRECOMP_MAX_FRAMES,
 * an explicit positional ROM, or SNESRECOMP_NO_LAUNCHER; those resolve the
 * ROM through the shared resolver and boot straight into the game.
 *
 * Input layout (runner 12-bit word per port, see debug_server
 * k_controller_names): B=0x001 Y=0x002 SELECT=0x004 START=0x008 UP=0x010
 * DOWN=0x020 LEFT=0x040 RIGHT=0x080 A=0x100 X=0x200 L=0x400 R=0x800. The
 * runner word packs player 1 in bits 0..11 and player 2 in bits 12..23;
 * RtlRunFrame feeds them to SNES ports 1 and 2 ($4218/$421A).
 * Keys (P1): Z=B X=A A=Y S=X Q=L E=R Enter=Start Backspace=Select arrows=D-pad.
 * The per-player layouts live in keybinds.ini [player1]/[player2] (edited
 * from the launcher's Controller page; restart to apply). Player 2 starts
 * unbound; pick its keys on the same page. Gamepad (when player_src[p] == 2):
 * bottom=B right=A left=Y top=X, L/R shoulders, Start/Select, D-pad, left
 * stick acts as D-pad. Pads are assigned to players in connection order.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "snes/snes.h"
#include "snes/ppu.h"
#include "snes/interp_bridge.h"
#include "snes/input_replay.h"
#include "common_cpu_infra.h"
#include "common_rtl.h"
#include "host_report.h"
#include "rom_image_verify.h"
#include "launcher.h"
#include "host_paths.h"
#include "super_tennis_rtl.h"
#include "cpu_state.h"
#include "spc_player.h"
#include "config.h"
#include "desktop/sdl_compat.h"

#if defined(RECOMP_LAUNCHER)
#include "recomp_launcher.h"   /* recomp_launcher_run_window() ABI */
#include "launcher_profile.h"  /* launcher_profile_apply("snes", &gi) */
#endif

#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
#include "runtime_ui.h"       /* in-game settings overlay adapter */
#include "runtime_ui_imgui.h" /* ImGui presentation glue (C++ TU) */

static SuperTennisRuntimeUi *g_runtime_ui;
static SuperTennisImGui *g_runtime_imgui;
#endif

SpcPlayer *SuperTennisSpcPlayerCreate(void);

void NORETURN Die(const char *error) {
  host_report_fatal(error);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Super Tennis recomp", error,
                           NULL);
  fprintf(stderr, "Error: %s\n", error);
  exit(1);
}

#define ST_FRAME_WIDTH 256
#define ST_FRAME_HEIGHT 224
#define ROM_DEFAULT "reference/Super Tennis (USA).sfc"

#if defined(RECOMP_LAUNCHER)
/* Borrowed by RecompLauncherCGameInfo.known_sha1_hex for the duration of
 * recomp_launcher_run_window(): the canonical SHA-1 identity of the
 * supported dump (SHA-1 of the headerless payload, exactly the bytes the
 * host and the launcher both hash). Lets the launcher's "ROM verified"
 * verdict and its can-launch gate agree with the host's SHA-256 gate. */
static const char *const k_rom_sha1_hex[] = {
    "9b2c6beb347c6487c170d4f6fc5ee537a223626e",
};
#endif

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static SDL_Texture *g_texture;
static uint8_t g_pixels[ST_FRAME_WIDTH * 4 * ST_FRAME_HEIGHT];
int g_ws_extra;

/* ── audio ─────────────────────────────────────────────────────────────── */

static SDL_Mutex *g_audio_mutex;
static SDL_AudioStream *g_audio_stream;
static uint8_t *g_audiobuffer;
static uint8_t *g_audiobuffer_cur;
static uint8_t *g_audiobuffer_end;
static int g_frames_per_block;
static uint8_t g_audio_channels;
static uint8_t *g_audio_stream_buffer;
static size_t g_audio_stream_buffer_size;

void RtlApuLock(void) {
  if (g_audio_mutex) SDL_LockMutex(g_audio_mutex);
}
void RtlApuUnlock(void) {
  if (g_audio_mutex) SDL_UnlockMutex(g_audio_mutex);
}

static void FillAudioBuffer(Uint8 *stream, int len) {
  SDL_LockMutex(g_audio_mutex);
  while (len != 0) {
    if (g_audiobuffer_end - g_audiobuffer_cur == 0) {
      RtlRenderAudio((int16 *)g_audiobuffer, g_frames_per_block,
                     g_audio_channels);
      g_audiobuffer_cur = g_audiobuffer;
      g_audiobuffer_end =
          g_audiobuffer + g_frames_per_block * g_audio_channels * sizeof(int16);
    }
    int n = (len < (int)(g_audiobuffer_end - g_audiobuffer_cur))
                ? len
                : (int)(g_audiobuffer_end - g_audiobuffer_cur);
    memcpy(stream, g_audiobuffer_cur, n);
    g_audiobuffer_cur += n;
    stream += n;
    len -= n;
  }
  SDL_UnlockMutex(g_audio_mutex);
}

static void SDLCALL AudioStreamCallback(void *userdata, SDL_AudioStream *stream,
                                        int additional_amount,
                                        int total_amount) {
  (void)userdata;
  (void)total_amount;
  if (additional_amount <= 0) return;
  if ((size_t)additional_amount > g_audio_stream_buffer_size) {
    uint8_t *resized =
        (uint8_t *)realloc(g_audio_stream_buffer, additional_amount);
    if (!resized) return;
    g_audio_stream_buffer = resized;
    g_audio_stream_buffer_size = (size_t)additional_amount;
  }
  FillAudioBuffer(g_audio_stream_buffer, additional_amount);
  SDL_PutAudioStreamData(stream, g_audio_stream_buffer, additional_amount);
}

/* `freq` comes from the launcher settings (default 48000). */
static int InitAudio(int freq) {
  g_audio_mutex = SDL_CreateMutex();
  if (!g_audio_mutex) {
    fprintf(stderr, "SDL_CreateMutex failed: %s\n", SDL_GetError());
    return 0;
  }
  SDL_AudioSpec want = {0}, have;
  want.freq = freq;
  want.format = SDL_AUDIO_S16;
  want.channels = 2;
  have = want;
  g_audio_stream = SDL_OpenAudioDeviceStream(
      SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, AudioStreamCallback, NULL);
  if (!g_audio_stream) {
    fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
    return 0;
  }
  SDL_GetAudioStreamFormat(g_audio_stream, &have, NULL);
  g_audio_channels = 2;
  /* Native DSP block is 534 samples at 32040 Hz. Round onto the opened rate
   * (32040->534 1:1, 48000->800, 44100->735). */
  RtlSetAudioOutputRate(have.freq);
  g_frames_per_block = (534 * have.freq + 32040 / 2) / 32040;
  g_audiobuffer =
      (uint8_t *)calloc(g_frames_per_block * g_audio_channels * sizeof(int16),
                        1);
  return 1;
}

/* ── input ─────────────────────────────────────────────────────────────── */

#define ST_PLAYER_COUNT 2

/* Live input bits per player (12-bit SNES controller word each). Packed into
 * the runner word as p1 | (p2 << 12) before each RtlRunFrame. */
static uint32 g_input_state[ST_PLAYER_COUNT];

static void SetKeyBit(int player, uint32 bit, bool down) {
  if (down) g_input_state[player] |= bit;
  else g_input_state[player] &= ~bit;
}

/* Keyboard layout per player: scancode -> runner input bit. Seeded with the
 * defaults below and overridden by keybinds.ini [player1]/[player2] (see
 * config.c). */
static uint32_t g_key_bind[ST_PLAYER_COUNT][SDL_NUM_SCANCODES];

#if SNESRECOMP_SDL3
#define ST_EVENT_SCANCODE(event) ((event).key.scancode)
#else
#define ST_EVENT_SCANCODE(event) ((event).key.keysym.scancode)
#endif

static void HandleKey(int player, SDL_Scancode sc, bool down) {
  if (player < 0 || player >= ST_PLAYER_COUNT) return;
  if ((int)sc >= 0 && (int)sc < SDL_NUM_SCANCODES) {
    uint32 bit = g_key_bind[player][sc];
    if (bit) SetKeyBit(player, bit, down);
  }
}

/* ── gamepad input (per player; active when player_src[p] == 2) ────────── */

typedef struct GamepadSlot {
  SDL_Gamepad *pad;   /* open when wanted && a device is assigned */
  SDL_JoystickID id;  /* assigned device; only meaningful while pad is open */
  bool wanted;        /* player_src[p] == 2 */
  Sint16 stick_x;
  Sint16 stick_y;
} GamepadSlot;

static GamepadSlot g_pads[ST_PLAYER_COUNT];

static SDL_JoystickID GamepadJoystickId(SDL_Gamepad *pad) {
#if SNESRECOMP_SDL3
  return SDL_GetGamepadID(pad);
#else
  return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));
#endif
}

/* Player slot holding `id`, or -1 when that device is not assigned. */
static int PadSlotForId(SDL_JoystickID id) {
  for (int p = 0; p < ST_PLAYER_COUNT; p++)
    if (g_pads[p].pad && g_pads[p].id == id) return p;
  return -1;
}

/* Assign `pad` to the first wanted player with a free slot (connection
 * order). Returns 1 when assigned; the caller closes it otherwise. */
static int AssignGamepad(SDL_Gamepad *pad, SDL_JoystickID id) {
  for (int p = 0; p < ST_PLAYER_COUNT; p++) {
    if (g_pads[p].wanted && !g_pads[p].pad) {
      g_pads[p].pad = pad;
      g_pads[p].id = id;
      g_pads[p].stick_x = 0;
      g_pads[p].stick_y = 0;
      fprintf(stderr, "gamepad assigned to player %d\n", p + 1);
      return 1;
    }
  }
  return 0;
}

static SDL_Gamepad *OpenGamepadDevice(SDL_JoystickID id) {
  SDL_Gamepad *pad = NULL;
#if SNESRECOMP_SDL3
  pad = SDL_OpenGamepad(id);
#else
  for (int i = 0; i < SDL_NumJoysticks(); ++i) {
    if (SDL_JoystickGetDeviceInstanceID(i) == id) {
      pad = SDL_GameControllerOpen(i);
      break;
    }
  }
#endif
  if (!pad) fprintf(stderr, "SDL_OpenGamepad failed: %s\n", SDL_GetError());
  return pad;
}

/* Open `id` into the first wanted player with a free slot; close it when no
 * player needs it. */
static void OpenGamepadForId(SDL_JoystickID id) {
  SDL_Gamepad *pad = OpenGamepadDevice(id);
  if (pad && !AssignGamepad(pad, id)) SDL_CloseGamepad(pad);
}

static void OpenConnectedGamepads(void) {
#if SNESRECOMP_SDL3
  int count = 0;
  SDL_JoystickID *pads = SDL_GetGamepads(&count);
  for (int i = 0; i < count; ++i)
    if (PadSlotForId(pads[i]) < 0) OpenGamepadForId(pads[i]);
  SDL_free(pads);
#else
  for (int i = 0; i < SDL_NumJoysticks(); ++i) {
    if (!SDL_IsGameController(i)) continue;
    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(i);
    if (PadSlotForId(id) >= 0) continue;
    SDL_Gamepad *pad = SDL_GameControllerOpen(i);
    if (!pad) {
      fprintf(stderr, "SDL_OpenGamepad failed: %s\n", SDL_GetError());
      continue;
    }
    if (!AssignGamepad(pad, GamepadJoystickId(pad))) SDL_CloseGamepad(pad);
  }
#endif
  for (int p = 0; p < ST_PLAYER_COUNT; p++)
    if (g_pads[p].wanted && !g_pads[p].pad)
      fprintf(stderr, "gamepad requested for player %d but none available\n",
              p + 1);
}

/* SDL3 renamed the controller EVENT constants (SDL_EVENT_CONTROLLER_* ->
 * SDL_EVENT_GAMEPAD_*); SDL2 only has the old names. The button/axis enums
 * are still provided under old-name aliases by both, so only events need
 * backend-gated names. */
#if SNESRECOMP_SDL3
#define ST_EVENT_DEVICE_ADDED    SDL_EVENT_GAMEPAD_ADDED
#define ST_EVENT_DEVICE_REMOVED  SDL_EVENT_GAMEPAD_REMOVED
#define ST_EVENT_BUTTON_DOWN     SDL_EVENT_GAMEPAD_BUTTON_DOWN
#define ST_EVENT_BUTTON_UP       SDL_EVENT_GAMEPAD_BUTTON_UP
#define ST_EVENT_AXIS_MOTION     SDL_EVENT_GAMEPAD_AXIS_MOTION
#else
#define ST_EVENT_DEVICE_ADDED    SDL_EVENT_CONTROLLER_DEVICE_ADDED
#define ST_EVENT_DEVICE_REMOVED  SDL_EVENT_CONTROLLER_DEVICE_REMOVED
#define ST_EVENT_BUTTON_DOWN     SDL_EVENT_CONTROLLER_BUTTON_DOWN
#define ST_EVENT_BUTTON_UP       SDL_EVENT_CONTROLLER_BUTTON_UP
#define ST_EVENT_AXIS_MOTION     SDL_EVENT_CONTROLLER_AXIS_MOTION
#endif

static uint32 GamepadButtonBit(int button) {
  switch (button) {
  case SDL_CONTROLLER_BUTTON_A: return 0x001; /* B (bottom) */
  case SDL_CONTROLLER_BUTTON_B: return 0x100; /* A (right) */
  case SDL_CONTROLLER_BUTTON_X: return 0x002; /* Y (left) */
  case SDL_CONTROLLER_BUTTON_Y: return 0x200; /* X (top) */
  case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return 0x400; /* L */
  case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return 0x800; /* R */
  case SDL_CONTROLLER_BUTTON_START: return 0x008;
  case SDL_CONTROLLER_BUTTON_BACK: return 0x004;
  case SDL_CONTROLLER_BUTTON_DPAD_UP: return 0x010;
  case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return 0x020;
  case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return 0x040;
  case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return 0x080;
  default: return 0;
  }
}

static void ApplyStickToDpad(int player, int deadzone_pct) {
  int dz = (deadzone_pct * 32767) / 100;
  if (dz < 0) dz = 0;
  if (dz > 32767) dz = 32767;
  SetKeyBit(player, 0x010, g_pads[player].stick_y < -dz); /* UP (stick up is negative Y) */
  SetKeyBit(player, 0x020, g_pads[player].stick_y > dz);  /* DOWN */
  SetKeyBit(player, 0x040, g_pads[player].stick_x < -dz); /* LEFT */
  SetKeyBit(player, 0x080, g_pads[player].stick_x > dz);  /* RIGHT */
}

/* ── rom loading ───────────────────────────────────────────────────────── */

static uint8_t *ReadWholeFile(const char *path, long *size_out) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *buf = (uint8_t *)malloc(size);
  if (!buf) { fclose(f); return NULL; }
  if (fread(buf, 1, size, f) != (size_t)size) { free(buf); fclose(f); return NULL; }
  fclose(f);
  *size_out = size;
  return buf;
}

/* Read the cached ROM path (rom.cfg beside the exe). Returns 1 when set. */
static int ReadCachedRomPath(char *out, size_t cap) {
  out[0] = '\0';
  FILE *f = fopen("rom.cfg", "r");
  if (!f) return 0;
  int ok = 0;
  if (fgets(out, (int)cap, f)) {
    size_t l = strlen(out);
    while (l && (out[l - 1] == '\n' || out[l - 1] == '\r')) out[--l] = '\0';
    ok = (l > 0);
  }
  fclose(f);
  if (!ok) out[0] = '\0';
  return ok;
}

static int DefaultRomExists(void) {
  FILE *f = fopen(ROM_DEFAULT, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

/* ── main ──────────────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
  static const uint8 expected_sha256[32] = {
      0x6e, 0x45, 0xa8, 0x0e, 0xa1, 0x48, 0x65, 0x45,
      0x14, 0xcb, 0x4e, 0x86, 0x04, 0xa0, 0xff, 0xcb,
      0xc7, 0x26, 0x94, 0x6e, 0x70, 0xf9, 0xe0, 0xb9,
      0x86, 0x0e, 0x36, 0xc0, 0xf3, 0xfa, 0x48, 0x77,
  };
  SuperTennisSettings settings;
  SuperTennisSettingsInitDefault(&settings);

  SnesInputReplay replay = {0};
  const char *replay_path = NULL;
  const char *positional_rom = NULL;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--replay") && i + 1 < argc && replay_path == NULL) {
      replay_path = argv[++i];
    } else if (argv[i][0] != '-' && positional_rom == NULL) {
      positional_rom = argv[i];
    } else {
      fprintf(stderr, "usage: %s [ROM] [--replay FILE]\n", argv[0]);
      return 2;
    }
  }

  long max_frames = -1;
  const char *mf_env = getenv("SNESRECOMP_MAX_FRAMES");
  if (mf_env && *mf_env) max_frames = strtol(mf_env, NULL, 10);

  char rom_path[1024];
  rom_path[0] = '\0';
  int rom_resolved = 0;

  /* Keyboard layout: defaults now; keybinds.ini [player1]/[player2] overrides
   * once the cwd is anchored to the exe dir below (RECOMP_LAUNCHER builds).
   * Player 2 starts unbound; the launcher's Controller page edits its keys. */
  SuperTennisKeyBindsDefaults(g_key_bind[0], SDL_NUM_SCANCODES);
  memset(g_key_bind[1], 0, sizeof(g_key_bind[1]));

  /* Resolver argument: an explicit positional ROM, else the historical
   * reference/ default when it is present. Absolutized BEFORE the cwd anchor
   * below so relative paths keep resolving against the launch directory
   * (host_paths contract). */
  char resolver_arg[1024];
  const char *resolver_rom = NULL;
  if (positional_rom) {
    if (snesrecomp_abspath(positional_rom, resolver_arg, sizeof(resolver_arg)))
      resolver_rom = resolver_arg;
    else
      resolver_rom = positional_rom;
  } else if (DefaultRomExists()) {
    if (snesrecomp_abspath(ROM_DEFAULT, resolver_arg, sizeof(resolver_arg)))
      resolver_rom = resolver_arg;
    else
      resolver_rom = ROM_DEFAULT;
  }

#if defined(RECOMP_LAUNCHER)
  /* Anchor cwd to the exe dir so rom.cfg, config.ini, keybinds.ini, saves/
   * and assets/ resolve beside the executable regardless of launch context. */
  snesrecomp_anchor_to_exe_dir();

  /* Persisted launcher settings (config.ini [Settings]); missing file keeps
   * the defaults above. Seed keybinds.ini with the host layout on first run
   * and (re)load it so the launcher's Controller page shows current binds. */
  SuperTennisSettingsLoad("config.ini", &settings);
  {
    FILE *probe = fopen("keybinds.ini", "r");
    if (probe) {
      fclose(probe);
    } else {
      SuperTennisKeyBindsWriteDefaults("keybinds.ini");
    }
  }
  {
    uint32_t *key_maps[ST_PLAYER_COUNT] = {g_key_bind[0], g_key_bind[1]};
    SuperTennisKeyBindsLoad("keybinds.ini", key_maps, SDL_NUM_SCANCODES);
  }

  /* Deterministic/scripted runs never show the window: --replay, an explicit
   * frame cap, a positional ROM, or SNESRECOMP_NO_LAUNCHER. */
  int headless = replay_path != NULL || max_frames > 0;
  const char *no_launcher = getenv("SNESRECOMP_NO_LAUNCHER");
  int want_launcher =
      !headless && !positional_rom && !(no_launcher && *no_launcher);

  /* skip_launcher: boot straight from the cached ROM when one exists (and
   * verifies); otherwise fall through and show the launcher so a fresh user
   * can still pick a ROM. */
  if (want_launcher && settings.skip_launcher) {
    char cached[1024];
    if (ReadCachedRomPath(cached, sizeof(cached)) &&
        snesrecomp_rom_verify_sha256(cached, expected_sha256)) {
      snprintf(rom_path, sizeof(rom_path), "%s", cached);
      rom_resolved = 1;
      want_launcher = 0;
    }
  }

  if (want_launcher) {
    RecompLauncherCSettings ls;
    memset(&ls, 0, sizeof(ls));
    ls.output_method = settings.output_method;
    ls.window_scale = settings.window_scale;
    ls.fullscreen = settings.fullscreen;
    ls.ignore_aspect = settings.ignore_aspect;
    ls.linear_filter = settings.linear_filter;
    ls.enable_audio = settings.enable_audio;
    ls.audio_freq = settings.audio_freq;
    ls.volume = settings.volume;
    ls.player_src[0] = settings.player_src[0];
    ls.player_src[1] = settings.player_src[1];
    ls.deadzone[0] = settings.deadzone[0];
    ls.deadzone[1] = settings.deadzone[1];
    ls.skip_launcher = settings.skip_launcher;

    char init_rom[1024] = "";
    if (!ReadCachedRomPath(init_rom, sizeof(init_rom)) && resolver_rom)
      snprintf(init_rom, sizeof(init_rom), "%s", resolver_rom);

    RecompLauncherCGameInfo gi;
    memset(&gi, 0, sizeof(gi));
    /* SNES system identity (platform "SUPER NINTENDO", CRT theme, ROM noun),
     * then the per-game specifics. Capabilities this host does not implement
     * are gated off so the launcher hides them. */
    launcher_profile_apply("snes", &gi);
    gi.name = "Super Tennis";
    gi.region = "(USA)";
    gi.sram_path = "saves/save.srm";
    gi.num_players = 2;
    gi.config_path = "config.ini";  /* hotkey editor target */
    gi.widescreen_supported = 0;
    gi.adaptive_view_supported = 0;
    gi.msu1_supported = 0;
    /* The same fingerprints this host gates on (expected_sha256 above),
     * so the launcher verifies the picked ROM instead of reporting the
     * valid dump as "not recognized". SHA-256 is the runtime gate; SHA-1
     * is the cartridge identity the recomp-ui SNES profile prefers. Both
     * digests cover the headerless payload, matching rom_image_verify.c
     * (and this dump carries no SMC header to strip). */
    gi.known_sha256 = &expected_sha256;
    gi.num_known_sha256 = 1;
    gi.known_sha1_hex = k_rom_sha1_hex;
    gi.num_known_sha1 = 1;

    int act = recomp_launcher_run_window(
        "Super Tennis \xE2\x80\x94 Launcher", &ls, &gi, ".",
        init_rom[0] ? init_rom : NULL, rom_path, sizeof(rom_path));
    fprintf(stderr, "launcher: action=%d rom=%s\n", act,
            rom_path[0] ? rom_path : "(none)");

    if (act == RECOMP_LAUNCHER_RESULT_QUIT) {
      return 0;  /* user closed the launcher */
    }
    if (act == RECOMP_LAUNCHER_RESULT_RELAUNCH) {
      /* This host has no toolchain/rebuild wizard, so the launcher cannot
       * request a relaunch; treat it as an exit. */
      fprintf(stderr, "launcher requested a rebuild/relaunch; not supported "
                      "by this host, exiting\n");
      return 0;
    }
    if (act == RECOMP_LAUNCHER_RESULT_LAUNCH) {
      settings.output_method = ls.output_method;
      settings.window_scale = ls.window_scale;
      settings.fullscreen = ls.fullscreen;
      settings.ignore_aspect = ls.ignore_aspect != 0;
      settings.linear_filter = ls.linear_filter != 0;
      settings.enable_audio = ls.enable_audio != 0;
      settings.audio_freq = ls.audio_freq;
      settings.volume = ls.volume;
      settings.player_src[0] = ls.player_src[0];
      settings.player_src[1] = ls.player_src[1];
      settings.deadzone[0] = ls.deadzone[0];
      settings.deadzone[1] = ls.deadzone[1];
      settings.skip_launcher = ls.skip_launcher != 0;
      /* Persist the launcher's choices so they survive the next boot. */
      SuperTennisSettingsSave("config.ini", &settings);
      if (rom_path[0]) {
        FILE *rc = fopen("rom.cfg", "w");
        if (rc) { fprintf(rc, "%s\n", rom_path); fclose(rc); }
        rom_resolved = 1;
      }
    }
    /* RECOMP_LAUNCHER_RESULT_UNAVAILABLE (2) falls through to the shared
     * resolver below, which retries rom.cfg then the native picker. */
  }
#endif

  if (!rom_resolved) {
    char *la_argv[2] = {(char *)"super-tennis",
                        (char *)(resolver_rom ? resolver_rom : "")};
    int la_argc = resolver_rom ? 2 : 1;
    if (!snesrecomp_launcher_resolve_rom_sha256(
            la_argc, la_argv, rom_path, sizeof(rom_path), expected_sha256)) {
      /* User cancelled the picker or repeatedly chose a non-matching ROM. */
      return 1;
    }
  }

  if (!snesrecomp_rom_verify_sha256(rom_path, expected_sha256)) {
    fprintf(stderr, "ROM verification failed: %s\n", rom_path);
    return 1;
  }
  long rom_size = 0;
  uint8_t *rom = ReadWholeFile(rom_path, &rom_size);
  if (!rom) {
    fprintf(stderr, "Unable to load ROM: %s\n", rom_path);
    return 1;
  }
  fprintf(stderr, "rom loaded: %s (%ld bytes)\n", rom_path, rom_size);

  /* SDL3's SDL_Init returns bool: true on success. */
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  for (int p = 0; p < ST_PLAYER_COUNT; p++)
    g_pads[p].wanted = (settings.player_src[p] == 2);
  OpenConnectedGamepads();

  int win_scale = settings.window_scale < 1 ? 1 : settings.window_scale;
  g_window = SDL_CreateWindow("Super Tennis (SNES recomp)",
                              ST_FRAME_WIDTH * win_scale,
                              ST_FRAME_HEIGHT * win_scale,
                              SDL_WINDOW_RESIZABLE);
  if (!g_window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return 1;
  }
  if (settings.fullscreen) snesrecomp_sdl_set_fullscreen(g_window, true);
  g_renderer = SDL_CreateRenderer(g_window, NULL);
  if (!g_renderer) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    return 1;
  }
  /* Sync presents to the display refresh. Without this the PPU frame is
   * scanned out mid-refresh: the title screen tears and the fine per-scanline
   * detail of the Mode-7 road shows up as horizontal-band "background
   * glitching" rather than a single clean tear. The reference host enables it
   * at renderer creation; we call SDL_CreateRenderer directly, so set it here.
   * Best-effort: if vsync isn't supported we fall back to the manual pacing
   * lower in the loop, which self-skips when present already consumed the
   * frame time. */
  SDL_SetRenderVSync(g_renderer, 1);
  SDL_SetRenderLogicalPresentation(
      g_renderer, ST_FRAME_WIDTH, ST_FRAME_HEIGHT,
      settings.ignore_aspect ? SDL_LOGICAL_PRESENTATION_STRETCH
                             : SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
  g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                ST_FRAME_WIDTH, ST_FRAME_HEIGHT);
  if (!g_texture) {
    fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
    return 1;
  }
  /* The PPU writes RGB as 0x00RRGGBB (alpha byte 0). Mark the texture opaque so
   * SDL3 presents those pixels instead of blending them as fully transparent
   * (which would show only the renderer's black clear color). Same call SMW's
   * host makes after texture creation. */
  snesrecomp_sdl_set_texture_opaque(g_texture);
  snesrecomp_sdl_set_texture_linear(g_texture, settings.linear_filter != 0);

  if (settings.enable_audio) {
    if (!InitAudio(settings.audio_freq)) {
      fprintf(stderr, "audio init failed; continuing without audio\n");
    } else {
#if SNESRECOMP_SDL3
      SDL_SetAudioStreamGain(g_audio_stream, (float)settings.volume / 100.0f);
#endif
    }
  }

  g_spc_player = SuperTennisSpcPlayerCreate();
  if (!g_spc_player) {
    fprintf(stderr, "SPC setup failed\n");
    return 1;
  }
  g_spc_player->initialize(g_spc_player);

  RtlRegisterGame(&kSuperTennisGameInfo);
  Snes *snes = SnesInit(rom, (int)rom_size);
  if (!snes) {
    fprintf(stderr, "SnesInit failed\n");
    return 1;
  }
  fprintf(stderr, "init: SnesInit ok\n");

  if (replay_path &&
      !snes_input_replay_open(&replay, replay_path, expected_sha256)) {
    fprintf(stderr, "Replay error: %s\n", snes_input_replay_error(&replay));
    return 1;
  }

  /* The callback renders through g_snes->apu, so do not let SDL invoke it
   * until SnesInit has established the emulated machine. */
  if (g_audio_stream) SDL_ResumeAudioStreamDevice(g_audio_stream);

  PpuBeginDrawing(g_ppu, g_pixels, (size_t)ST_FRAME_WIDTH * 4,
                  kPpuRenderFlags_NewRenderer);
  fprintf(stderr, "init: PpuBeginDrawing ok\n");

#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
  /* In-game settings overlay: a fresh ImGui context bound to the game window
   * (the pre-boot launcher destroyed its own before returning) plus the
   * shared runtime menu over the host settings. */
  g_runtime_imgui = st_imgui_create(g_window, g_renderer);
  if (g_runtime_imgui) {
    g_runtime_ui = SuperTennisRuntimeUiCreate(&settings, g_window, g_renderer,
                                              g_texture, g_audio_stream);
    if (g_runtime_ui) {
      fprintf(stderr, "init: runtime settings overlay ready (F1)\n");
      /* Smoke-test hook (mirrors SNESRECOMP_MAX_FRAMES): boot with the menu
       * open so headless CI can exercise the overlay render path. */
      const char *open_env = getenv("SNESRECOMP_OPEN_OVERLAY");
      if (open_env && *open_env) SuperTennisRuntimeUiOpen(g_runtime_ui);
    } else {
      st_imgui_destroy(g_runtime_imgui);
      g_runtime_imgui = NULL;
    }
  }
#endif

  fprintf(stderr, "entering main loop\n");
  /* Headless/scripted runs: SNESRECOMP_MAX_FRAMES exits cleanly after N
   * frames so the tier2 coverage manifest is flushed for offline ingest. */
  bool running = true;
  long host_frame_number = 0;
  uint32 last_tick = SDL_GetTicks();
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
  bool prev_overlay_open = false;
#endif
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        running = false;
        break;
      case SDL_EVENT_KEY_DOWN: {
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
        /* Menu input first (repeats reach the menu for value stepping); a
         * consumed event never reaches the game. While the menu is open even
         * unknown keys are withheld, so ESC backs out of the menu instead of
         * quitting. */
        if (g_runtime_ui &&
            SuperTennisRuntimeUiHandleEvent(g_runtime_ui, &event))
          break;
#endif
        if (event.key.repeat) break;
        if (event.key.key == SDLK_ESCAPE) running = false;
        for (int p = 0; p < ST_PLAYER_COUNT; p++)
          if (settings.player_src[p] == 1)
            HandleKey(p, ST_EVENT_SCANCODE(event), true);
        break;
      }
      case SDL_EVENT_KEY_UP:
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
        if (g_runtime_ui &&
            SuperTennisRuntimeUiHandleEvent(g_runtime_ui, &event))
          break;
#endif
        for (int p = 0; p < ST_PLAYER_COUNT; p++)
          if (settings.player_src[p] == 1)
            HandleKey(p, ST_EVENT_SCANCODE(event), false);
        break;
      case ST_EVENT_DEVICE_ADDED:
        /* Opens into the first wanted player with a free slot; a device no
         * player wants is opened then closed. */
        OpenGamepadForId(SNESRECOMP_SDL_EVENT_DEVICE(event));
        break;
      case ST_EVENT_DEVICE_REMOVED: {
        int slot = PadSlotForId(SNESRECOMP_SDL_EVENT_DEVICE(event));
        if (slot >= 0) {
          SDL_CloseGamepad(g_pads[slot].pad);
          g_pads[slot].pad = NULL;
          g_pads[slot].stick_x = 0;
          g_pads[slot].stick_y = 0;
        }
        break;
      }
      case ST_EVENT_BUTTON_DOWN:
      case ST_EVENT_BUTTON_UP: {
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
        if (g_runtime_ui &&
            SuperTennisRuntimeUiHandleEvent(g_runtime_ui, &event))
          break;
#endif
        int slot = PadSlotForId(SNESRECOMP_SDL_EVENT_BUTTON_DEVICE(event));
        if (slot < 0) break;
        uint32 bit = GamepadButtonBit(SNESRECOMP_SDL_EVENT_BUTTON(event));
        if (bit) SetKeyBit(slot, bit, event.type == ST_EVENT_BUTTON_DOWN);
        break;
      }
      case ST_EVENT_AXIS_MOTION:
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
        if (g_runtime_ui &&
            SuperTennisRuntimeUiHandleEvent(g_runtime_ui, &event))
          break;
#endif
        int slot = PadSlotForId(SNESRECOMP_SDL_EVENT_AXIS_DEVICE(event));
        if (slot < 0) break;
        if (SNESRECOMP_SDL_EVENT_AXIS(event) == SDL_CONTROLLER_AXIS_LEFTX)
          g_pads[slot].stick_x = SNESRECOMP_SDL_EVENT_AXIS_VALUE(event);
        else if (SNESRECOMP_SDL_EVENT_AXIS(event) == SDL_CONTROLLER_AXIS_LEFTY)
          g_pads[slot].stick_y = SNESRECOMP_SDL_EVENT_AXIS_VALUE(event);
        else
          break;
        ApplyStickToDpad(slot, settings.deadzone[slot]);
        break;
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
      case SDL_EVENT_MOUSE_MOTION:
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
      case SDL_EVENT_MOUSE_WHEEL:
        /* Mouse only matters to the menu; forward it while the overlay is
         * open so sections/rows/buttons are clickable. */
        if (g_runtime_imgui && g_runtime_ui &&
            SuperTennisRuntimeUiIsOpen(g_runtime_ui))
          st_imgui_process_event(g_runtime_imgui, &event);
        break;
#endif
      default:
        break;
      }
    }

    /* Menu open pauses simulation: no new frames, no audio feed, and every
     * key/button is withheld by SuperTennisRuntimeUiHandleEvent above. The
     * last frame keeps displaying under the overlay. */
    bool overlay_open =
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
        g_runtime_ui && SuperTennisRuntimeUiIsOpen(g_runtime_ui);
#else
        false;
#endif
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
    /* A game key/button held when the menu opened was released into the menu
     * (withheld), so its game-input bit would otherwise stay set forever. */
    if (prev_overlay_open && !overlay_open) {
      for (int p = 0; p < ST_PLAYER_COUNT; p++) {
        g_input_state[p] = 0;
        g_pads[p].stick_x = 0;
        g_pads[p].stick_y = 0;
      }
    }
    prev_overlay_open = overlay_open;
#endif
    if (!overlay_open) {
      ++host_frame_number;
      /* Player 2 sits in bits 12..23 of the runner word; a player whose
       * source is None keeps its word zero, so the pack is always safe. */
      uint32 frame_input = g_input_state[0] | (g_input_state[1] << 12);
      if (replay_path &&
          !snes_input_replay_next(&replay, NULL, &frame_input)) {
        running = false;
        break;
      }
      RtlRunFrame(frame_input);
      if (!SuperTennisLastFrameOk()) {
        fprintf(stderr,
                "Super Tennis execution failed at host frame %ld "
                "(game frame %d) input=$%03X\n",
                host_frame_number, snes_frame_counter, frame_input);
        interp_bridge_dump_recent_steps(256, stderr);
        running = false;
      }

      if (max_frames > 0 && host_frame_number >= max_frames)
        running = false;

      SuperTennisDrawPpuFrame();
      SDL_UpdateTexture(g_texture, NULL, g_pixels,
                        (int)(ST_FRAME_WIDTH * 4));
    }

    SDL_RenderClear(g_renderer);
    SDL_RenderTexture(g_renderer, g_texture, NULL, NULL);
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
    if (overlay_open && g_runtime_imgui)
      /* Draws the menu and restores the full-target viewport + 256x224
       * logical presentation before returning (SDL3 reinterprets a stale
       * concrete viewport in logical coordinates once the scale changes,
       * which would zoom the picture into the top-left corner). */
      st_imgui_render_overlay(g_runtime_imgui, g_runtime_ui, g_renderer);
#endif
    SDL_RenderPresent(g_renderer);

    /* Live input-source switch from the overlay menu. */
    for (int p = 0; p < ST_PLAYER_COUNT; p++) {
      bool want_pad = settings.player_src[p] == 2;
      if (want_pad != g_pads[p].wanted) {
        g_pads[p].wanted = want_pad;
        if (!want_pad) {
          if (g_pads[p].pad) {
            SDL_CloseGamepad(g_pads[p].pad);
            g_pads[p].pad = NULL;
            g_pads[p].stick_x = 0;
            g_pads[p].stick_y = 0;
          }
        } else {
          OpenConnectedGamepads();
        }
      }
    }

    /* ~60 fps pacing (17/17/16 ms) so audio stays in sync. */
    {
      static const uint8 delays[3] = {17, 17, 16};
      static unsigned delay_index;
      uint32 cur = SDL_GetTicks();
      uint32 delay = delays[delay_index];
      delay_index = (delay_index + 1) % 3;
      uint32 target = last_tick + delay;
      last_tick += delay;
      if (target > cur) {
        uint32 delta = target - cur;
        if (delta > 500) delta = 500;
        SDL_Delay(delta);
      } else if (cur - target > 500) {
        last_tick = cur;
      }
    }
  }

  Tier2CoverageWriteDefaultManifest("super-tennis");

  for (int p = 0; p < ST_PLAYER_COUNT; p++)
    if (g_pads[p].pad) SDL_CloseGamepad(g_pads[p].pad);
#if defined(RECOMP_LAUNCHER) && SNESRECOMP_SDL3
  if (g_runtime_ui) SuperTennisRuntimeUiDestroy(g_runtime_ui);
  if (g_runtime_imgui) st_imgui_destroy(g_runtime_imgui);
#endif
  SDL_DestroyTexture(g_texture);
  SDL_DestroyRenderer(g_renderer);
  SDL_DestroyWindow(g_window);
  SDL_Quit();
  snes_input_replay_close(&replay);
  free(rom);
  return 0;
}
