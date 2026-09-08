#include "super_tennis_rtl.h"

#include "audio_trace.h"
#include "common_rtl.h"
#include "rom_image_verify.h"
#include "snes/interp_bridge.h"
#include "snes/input_replay.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "spc_player.h"
#include "types.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SpcPlayer *SuperTennisSpcPlayerCreate(void);

/* Single-threaded headless host: there is no audio callback to synchronize. */
void RtlApuLock(void) {}
void RtlApuUnlock(void) {}
void host_report_breadcrumb(const char *format, ...) { (void)format; }

int g_ws_extra;

enum { kFrameWidth = 256, kFrameHeight = 224 };
static uint8 s_frame_pixels[kFrameWidth * kFrameHeight * 4];

void NORETURN Die(const char *message) {
  fprintf(stderr, "super-tennis-headless: fatal: %s\n",
          message ? message : "(unknown)");
  exit(1);
}

static uint8 *read_file(const char *path, size_t *size_out) {
  FILE *file = fopen(path, "rb");
  long length;
  uint8 *data;
  if (!file)
    return NULL;
  if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0 ||
      fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return NULL;
  }
  data = malloc((size_t)length);
  if (!data || fread(data, 1, (size_t)length, file) != (size_t)length) {
    free(data);
    fclose(file);
    return NULL;
  }
  fclose(file);
  *size_out = (size_t)length;
  return data;
}

static const char *audio_event_name(uint8 type) {
  switch (type) {
  case AUDIO_TRACE_EV_CPU_PORT_WRITE: return "cpu_write";
  case AUDIO_TRACE_EV_SPC_PORT_READ: return "spc_read";
  case AUDIO_TRACE_EV_SPC_PORT_WRITE: return "spc_write";
  case AUDIO_TRACE_EV_CPU_PORT_READ: return "cpu_read";
  case AUDIO_TRACE_EV_CPU_PORT_APPLY: return "cpu_apply";
  default: return NULL;
  }
}

static bool write_audio_port_events(const char *path) {
  AudioTraceStats stats;
  AudioTraceEvent events[4096];
  uint64 index = 0;
  uint64 oldest = 0;
  FILE *file = fopen(path, "wbx");
  if (!file) {
    fprintf(stderr, "super-tennis-headless: cannot create audio trace '%s'\n",
            path);
    return false;
  }
  audio_trace_get_stats(&stats);
  while (index < stats.event_count) {
    uint32 count = audio_trace_copy_events(index, 4096, events, &oldest);
    if (index < oldest)
      index = oldest;
    if (!count)
      break;
    for (uint32 i = 0; i < count; ++i, ++index) {
      const char *name = audio_event_name(events[i].type);
      if (!name)
        continue;
      if (fprintf(file,
                  "{\"schema_version\":1,\"source\":\"snesrecomp\","
                  "\"event_index\":%llu,\"sample_index\":%llu,"
                  "\"frame\":%u,\"event\":\"%s\",\"port\":%u,"
                  "\"value\":%u,\"producer\":%u}\n",
                  (unsigned long long)index,
                  (unsigned long long)events[i].sample_idx, events[i].aux,
                  name, events[i].addr, events[i].val,
                  events[i].producer) < 0) {
        fclose(file);
        return false;
      }
    }
  }
  if (fclose(file) != 0)
    return false;
  return true;
}

static bool write_frame_ppm(const char *path) {
  FILE *file = fopen(path, "wb");
  if (!file) {
    fprintf(stderr, "super-tennis-headless: cannot create frame image '%s'\n",
            path);
    return false;
  }
  if (fprintf(file, "P6\n%d %d\n255\n", kFrameWidth, kFrameHeight) < 0) {
    fclose(file);
    return false;
  }
  for (size_t offset = 0; offset < sizeof(s_frame_pixels); offset += 4) {
    uint32 pixel;
    memcpy(&pixel, s_frame_pixels + offset, sizeof(pixel));
    if (fputc((pixel >> 16) & 0xff, file) == EOF ||
        fputc((pixel >> 8) & 0xff, file) == EOF ||
        fputc(pixel & 0xff, file) == EOF) {
      fclose(file);
      return false;
    }
  }
  return fclose(file) == 0;
}

static void usage(const char *program) {
  fprintf(stderr,
          "usage: %s [ROM] [--frames N] "
          "[--capture-dir DIR --capture-nmi N] [--audio-events FILE] "
          "[--frame-ppm FILE] [--replay FILE]\n",
          program);
}

int main(int argc, char **argv) {
  static const uint8 expected_sha256[32] = {
      0x6e, 0x45, 0xa8, 0x0e, 0xa1, 0x48, 0x65, 0x45,
      0x14, 0xcb, 0x4e, 0x86, 0x04, 0xa0, 0xff, 0xcb,
      0xc7, 0x26, 0x94, 0x6e, 0x70, 0xf9, 0xe0, 0xb9,
      0x86, 0x0e, 0x36, 0xc0, 0xf3, 0xfa, 0x48, 0x77,
  };
  const char *rom_path = "reference/Super Tennis (USA).sfc";
  const char *capture_dir = NULL;
  const char *audio_events_path = NULL;
  const char *frame_ppm_path = NULL;
  unsigned long frames = 1;
  bool frames_explicit = false;
  unsigned long capture_nmi = 0;
  const char *replay_path = NULL;
  SnesInputReplay replay = {0};

  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) {
      char *end = NULL;
      errno = 0;
      frames = strtoul(argv[++i], &end, 10);
      if (errno || !end || *end || !frames) {
        usage(argv[0]);
        return 2;
      }
      frames_explicit = true;
    } else if (!strcmp(argv[i], "--capture-dir") && i + 1 < argc) {
      capture_dir = argv[++i];
    } else if (!strcmp(argv[i], "--audio-events") && i + 1 < argc) {
      audio_events_path = argv[++i];
    } else if (!strcmp(argv[i], "--frame-ppm") && i + 1 < argc) {
      frame_ppm_path = argv[++i];
    } else if (!strcmp(argv[i], "--replay") && i + 1 < argc) {
      if (replay_path != NULL) {
        usage(argv[0]);
        return 2;
      }
      replay_path = argv[++i];
    } else if (!strcmp(argv[i], "--capture-nmi") && i + 1 < argc) {
      char *end = NULL;
      errno = 0;
      capture_nmi = strtoul(argv[++i], &end, 10);
      if (errno || !end || *end || !capture_nmi ||
          capture_nmi > UINT32_MAX) {
        usage(argv[0]);
        return 2;
      }
    } else if (argv[i][0] == '-') {
      usage(argv[0]);
      return 2;
    } else {
      rom_path = argv[i];
    }
  }

  if ((capture_dir == NULL) != (capture_nmi == 0)) {
    usage(argv[0]);
    return 2;
  }
  if (capture_dir &&
      !SuperTennisConfigureCapture(capture_dir, (unsigned)capture_nmi))
    return 2;

  if (!snesrecomp_rom_verify_sha256(rom_path, expected_sha256))
    return 1;
  if (replay_path) {
    if (!snes_input_replay_open(&replay, replay_path, expected_sha256)) {
      fprintf(stderr, "super-tennis-headless: replay error: %s\n",
              snes_input_replay_error(&replay));
      return 1;
    }
    if (!frames_explicit)
      frames = replay.sample_count;
    if (frames > replay.sample_count) {
      fprintf(stderr,
              "super-tennis-headless: requested %lu frames but replay has %u\n",
              frames, replay.sample_count);
      snes_input_replay_close(&replay);
      return 2;
    }
  }

  size_t rom_size = 0;
  uint8 *rom = read_file(rom_path, &rom_size);
  if (!rom) {
    fprintf(stderr, "super-tennis-headless: cannot read '%s'\n", rom_path);
    snes_input_replay_close(&replay);
    return 1;
  }

  RtlRegisterGame(&kSuperTennisGameInfo);
  if (!SnesInit(rom, (int)rom_size)) {
    fprintf(stderr, "super-tennis-headless: ROM load failed\n");
    snes_input_replay_close(&replay);
    free(rom);
    return 1;
  }
  if (frame_ppm_path) {
    PpuBeginDrawing(g_ppu, s_frame_pixels, (size_t)kFrameWidth * 4,
                    kPpuRenderFlags_NewRenderer);
  }

  g_spc_player = SuperTennisSpcPlayerCreate();
  if (!g_spc_player) {
    fprintf(stderr, "super-tennis-headless: SPC setup failed\n");
    snes_input_replay_close(&replay);
    free(rom);
    return 1;
  }
  g_spc_player->initialize(g_spc_player);

  for (unsigned long frame = 0; frame < frames; ++frame) {
    unsigned long frame_number = frame + 1;
    uint32 input = 0;
    SnesInputReplaySample replay_sample;
    if (replay_path &&
        !snes_input_replay_next(&replay, &replay_sample, &input)) {
      fprintf(stderr,
              "super-tennis-headless: replay ended before frame %lu\n",
              frame_number);
      snes_input_replay_close(&replay);
      free(rom);
      return 1;
    }
    RtlRunFrame(input);
    if (!SuperTennisLastFrameOk()) {
      fprintf(stderr,
              "super-tennis-headless: execution failed at frame %lu "
              "input=$%03X resume=$%06X\n",
              frame_number, input, SuperTennisResumePc());
      interp_bridge_dump_recent_steps(256, stderr);
      snes_input_replay_close(&replay);
      free(rom);
      return 1;
    }
    if (frame_ppm_path)
      SuperTennisDrawPpuFrame();
  }

  if (audio_events_path && !write_audio_port_events(audio_events_path)) {
    snes_input_replay_close(&replay);
    free(rom);
    return 1;
  }
  if (frame_ppm_path && !write_frame_ppm(frame_ppm_path)) {
    snes_input_replay_close(&replay);
    free(rom);
    return 1;
  }

  printf("Super Tennis headless smoke completed %lu frame%s; "
         "resume=%06X S=%04X M%uX%u beam=%u:%u master=%llu tier2=%ld "
         "nmis=%u replay_polls=%u "
         "last_nmi={master:%llu,S:%04X,A:%04X,Y:%04X,P:%02X}\n",
         frames, frames == 1 ? "" : "s", SuperTennisResumePc(),
         g_cpu.S, g_cpu.m_flag, g_cpu.x_flag,
         g_snes->vPos, g_snes->hPos,
         (unsigned long long)g_cpu.master_cycles, interp_tier_hit_count(),
         SuperTennisNmiCount(), replay.next_sample, SuperTennisLastNmiMaster(),
         SuperTennisLastNmiStack(), SuperTennisLastNmiA(),
         SuperTennisLastNmiY(), SuperTennisLastNmiP());
  snes_input_replay_close(&replay);
  free(rom);
  return 0;
}
