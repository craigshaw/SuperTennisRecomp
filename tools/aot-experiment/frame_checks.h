/* Private experiment host only. Matches the accepted frame CSV contract. */
#include "snes/apu.h"
static uint64_t validation_hash(const void *data, size_t n) {
  const unsigned char *p = data;
  uint64_t h = UINT64_C(14695981039346656037);
  while (n--) { h ^= *p++; h *= UINT64_C(1099511628211); }
  return h;
}
static void validation_frame(FILE *f, unsigned long frame, uint32 input) {
  char row[1024];
  snprintf(row, sizeof(row), "%lu,%06X,%06X,%u,%u,%04X,%u,%u,%llu,%u,%016llx,%016llx,%016llx,%016llx,%016llx,%016llx,%016llx,%016llx\n",
    frame, input, SuperTennisResumePc(), g_cpu.m_flag, g_cpu.x_flag,
    g_cpu.S, g_snes->vPos, g_snes->hPos,
    (unsigned long long)g_cpu.master_cycles, SuperTennisNmiCount(),
    (unsigned long long)validation_hash(g_ram, sizeof(g_ram)),
    (unsigned long long)validation_hash(g_ram+0x2000, sizeof(g_ram)-0x2000),
    (unsigned long long)validation_hash(g_ppu->vram, sizeof(g_ppu->vram)),
    (unsigned long long)validation_hash(g_ppu->cgram, sizeof(g_ppu->cgram)),
    (unsigned long long)validation_hash(g_ppu->oam, sizeof(g_ppu->oam)),
    (unsigned long long)validation_hash(g_ppu->highOam, sizeof(g_ppu->highOam)),
    (unsigned long long)validation_hash(g_snes->apu->ram, sizeof(g_snes->apu->ram)),
    (unsigned long long)validation_hash(s_frame_pixels, sizeof(s_frame_pixels)));
  fputs(row, f);
  static FILE *reference;
  static int initialized;
  if (!initialized) {
    initialized = 1;
    const char *path = getenv("ST_FRAME_REFERENCE");
    if (path) {
      reference = fopen(path, "rb");
      char header[1024];
      if (!reference || !fgets(header, sizeof(header), reference)) Die("cannot read frame reference");
    }
  }
  if (reference) {
    char expected[1024];
    if (!fgets(expected, sizeof(expected), reference) || strcmp(row, expected)) {
      fprintf(stderr, "frame comparison differs at frame %lu\n", frame);
      fflush(f);
      exit(3);
    }
  }

}
