/* Minimal framework SPC facade. Super Tennis uploads its own SPC program
 * through the SNES APU ports, so the hardware model owns the actual upload. */
#include "spc_player.h"
#include "snes/spc.h"
#include "snes/dsp_regs.h"

#include <stdlib.h>
#include <string.h>

typedef struct SuperTennisSpcPlayer {
  SpcPlayer base;
  uint8 ram[65536];
} SuperTennisSpcPlayer;

static void write_dsp(SuperTennisSpcPlayer *player, uint8 reg, uint8 value) {
  if (player->base.dsp)
    dsp_write(player->base.dsp, reg, value);
}

static void initialize(SpcPlayer *base) {
  SuperTennisSpcPlayer *player = (SuperTennisSpcPlayer *)base;
  static const uint8 regs[12] = {
      MVOLL, MVOLR, EVOLL, EVOLR, FLG, EFB,
      PMON, NON, EON, DIR, ESA, EDL,
  };
  static const uint8 values[12] = {
      0x7f, 0x7f, 0, 0, 0x2f, 0x60, 0, 0, 0, 0x80, 0x60, 2,
  };

  if (player->base.dsp)
    dsp_reset(player->base.dsp);
  memset(player->ram, 0, 0x500);
  memset(player->base.input_ports, 0, sizeof(player->base.input_ports));
  for (int i = 11; i >= 0; --i)
    write_dsp(player, regs[i], values[i]);
}

static void upload(SpcPlayer *base, const uint8 *data) {
  (void)base;
  (void)data;
}

SpcPlayer *g_spc_player;

SpcPlayer *SuperTennisSpcPlayerCreate(void) {
  SuperTennisSpcPlayer *player = calloc(1, sizeof(*player));
  if (!player)
    return NULL;
  player->base.dsp = dsp_init(player->ram);
  player->base.initialize = initialize;
  player->base.upload = upload;
  return &player->base;
}
