/*
 * First ROM-only frame driver for Super Tennis.
 *
 * There is deliberately no hand-decompiled game loop here. The reset path,
 * NMI, and mainline continuation run through the whole-program bridge, which
 * bounces to generated AOT functions whenever the exact PC/M/X variant is in
 * the manifest and interprets everything else.
 */
#include "super_tennis_rtl.h"

#include "common_cpu_infra.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include "snes/interp_bridge.h"
#include "snes/apu.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/snes_regs.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  kResetPc = 0x008000u,
  kNmiPc = 0x008917u,
  kIrqPc = 0x00c175u,
  kRdnmiPollPc = 0x00c7b1u,
  kVblankLine = 225u,
  kMasterClocksPerField = 1364u * 262u,
};

static bool s_booted;
static bool s_ok = true;
static bool s_vblank_due;
static uint32 s_resume_pc;
static unsigned s_nmi_count;
static unsigned s_frame_count;
static uint16 s_last_nmi_s;
static uint16 s_last_nmi_a;
static uint16 s_last_nmi_y;
static uint8 s_last_nmi_p;
static uint64 s_last_nmi_master;
static const char *s_capture_directory;
static unsigned s_capture_nmi;
static unsigned s_active_nmis[256];
static unsigned s_active_nmi_depth;
static bool s_atomic_irq_active;

static bool make_capture_path(char *path, size_t capacity,
                              unsigned nmi, const char *boundary,
                              const char *suffix) {
  int length = snprintf(path, capacity, "%s/nmi-%04u-%s.%s",
                        s_capture_directory, nmi, boundary, suffix);
  return length > 0 && (size_t)length < capacity;
}

static FILE *open_capture_file(unsigned nmi, const char *boundary,
                               const char *suffix, char *path,
                               size_t path_capacity) {
  if (!make_capture_path(path, path_capacity, nmi, boundary, suffix)) {
    fprintf(stderr, "super-tennis-headless: capture path is too long\n");
    return NULL;
  }
  FILE *file = fopen(path, "wbx");
  if (!file)
    fprintf(stderr, "super-tennis-headless: cannot create capture '%s'\n",
            path);
  return file;
}

static bool write_bytes(unsigned nmi, const char *boundary, const char *suffix,
                        const void *data, size_t size) {
  char path[1024];
  FILE *file = open_capture_file(nmi, boundary, suffix, path, sizeof(path));
  if (!file)
    return false;
  bool ok = fwrite(data, 1, size, file) == size && fclose(file) == 0;
  if (!ok)
    fprintf(stderr, "super-tennis-headless: incomplete capture '%s'\n", path);
  return ok;
}

static bool write_words_le(unsigned nmi, const char *boundary,
                           const char *suffix, const uint16_t *words,
                           size_t count, const uint8_t *tail,
                           size_t tail_size) {
  char path[1024];
  FILE *file = open_capture_file(nmi, boundary, suffix, path, sizeof(path));
  if (!file)
    return false;
  bool ok = true;
  for (size_t i = 0; i < count && ok; ++i) {
    uint8_t bytes[2] = {(uint8_t)words[i], (uint8_t)(words[i] >> 8)};
    ok = fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
  }
  if (ok && tail_size)
    ok = fwrite(tail, 1, tail_size, file) == tail_size;
  ok = fclose(file) == 0 && ok;
  if (!ok)
    fprintf(stderr, "super-tennis-headless: incomplete capture '%s'\n", path);
  return ok;
}

static bool write_machine_capture(unsigned nmi, const char *boundary,
                                  uint32 pc24) {
  Apu *apu = g_snes->apu;
  Spc *spc = apu->spc;
  uint8_t ports[8];
  char state_path[1024];
  FILE *state;

  memcpy(ports, apu->inPorts, 4);
  memcpy(ports + 4, apu->outPorts, 4);
  if (!write_bytes(nmi, boundary, "wram.bin", g_ram, sizeof(g_ram)) ||
      !write_words_le(nmi, boundary, "vram.bin", g_ppu->vram, 0x8000,
                      NULL, 0) ||
      !write_words_le(nmi, boundary, "cgram.bin", g_ppu->cgram, 0x100,
                      NULL, 0) ||
      !write_words_le(nmi, boundary, "oam.bin", g_ppu->oam, 0x100,
                      g_ppu->highOam, sizeof(g_ppu->highOam)) ||
      !write_bytes(nmi, boundary, "apuram.bin", apu->ram,
                   sizeof(apu->ram)) ||
      !write_bytes(nmi, boundary, "apuports.bin", ports, sizeof(ports)))
    return false;

  state = open_capture_file(nmi, boundary, "state.json", state_path,
                            sizeof(state_path));
  if (!state)
    return false;
  int written = fprintf(
      state,
      "{\"schema_version\":1,\"source\":\"snesrecomp\","
      "\"boundary\":\"%s\",\"frame\":%u,\"nmi\":%u,"
      "\"master_cycle\":%llu,\"pc24\":%u,"
      "\"cpu\":{\"a\":%u,\"x\":%u,\"y\":%u,\"sp\":%u,"
      "\"d\":%u,\"dbr\":%u,\"k\":%u,\"ps\":%u,"
      "\"emulation_mode\":%s},"
      "\"spc\":{\"a\":%u,\"x\":%u,\"y\":%u,\"sp\":%u,"
      "\"pc\":%u,\"ps\":%u,\"port_clock\":%llu},"
      "\"regions\":{\"wram\":131072,\"vram\":65536,"
      "\"cgram\":512,\"oam\":544,\"apuram\":65536,"
      "\"apuports\":8}}\n",
      boundary, s_frame_count, nmi,
      (unsigned long long)g_cpu.master_cycles, pc24, g_cpu.A, g_cpu.X,
      g_cpu.Y, g_cpu.S, g_cpu.D, g_cpu.DB, (unsigned)(pc24 >> 16), g_cpu.P,
      g_cpu.emulation ? "true" : "false", spc->a, spc->x, spc->y, spc->sp,
      spc->pc,
      (spc->n ? 0x80 : 0) | (spc->v ? 0x40 : 0) | (spc->p ? 0x20 : 0) |
          (spc->b ? 0x10 : 0) | (spc->h ? 0x08 : 0) |
          (spc->i ? 0x04 : 0) | (spc->z ? 0x02 : 0) |
          (spc->c ? 0x01 : 0),
      (unsigned long long)apu->portClock);
  bool ok = written > 0 && fclose(state) == 0;
  if (!ok)
    fprintf(stderr, "super-tennis-headless: incomplete capture '%s'\n",
            state_path);
  return ok;
}

static bool capture_entry_if_requested(void) {
  if (!s_capture_directory ||
      (s_nmi_count != s_capture_nmi &&
       s_nmi_count != s_capture_nmi + 1u))
    return true;
  return write_machine_capture(s_nmi_count, "entry", kNmiPc);
}

static void capture_interrupt_return(CpuState *cpu, uint32_t return_pc) {
  (void)cpu;
  if (s_atomic_irq_active || !s_active_nmi_depth)
    return;
  unsigned completed_nmi = s_active_nmis[--s_active_nmi_depth];
  if (s_capture_directory && completed_nmi == s_capture_nmi &&
      !write_machine_capture(completed_nmi, "exit", return_pc))
    s_ok = false;
}

int SuperTennisConfigureCapture(const char *directory, unsigned nmi_number) {
  if (!directory || !directory[0] || !nmi_number ||
      nmi_number == UINT32_MAX) {
    fprintf(stderr, "super-tennis-headless: invalid capture configuration\n");
    return 0;
  }
  s_capture_directory = directory;
  s_capture_nmi = nmi_number;
  return 1;
}

static bool write_first_nmi_snapshot(void) {
  const char *path = getenv("SNESRECOMP_ST_NMI_SNAPSHOT");
  if (!path || !path[0] || s_nmi_count != 1)
    return true;
  FILE *output = fopen(path, "wbx");
  if (!output) {
    fprintf(stderr, "super-tennis-headless: cannot create NMI snapshot '%s'\n",
            path);
    return false;
  }
  bool ok = fwrite(g_ram, 1, sizeof(g_ram), output) == sizeof(g_ram) &&
            fclose(output) == 0;
  if (!ok)
    fprintf(stderr, "super-tennis-headless: incomplete NMI snapshot '%s'\n",
            path);
  return ok;
}

static void prepare_interrupt(CpuState *cpu, uint32 return_pc) {
  cpu_push_interrupt_frame_at(cpu, return_pc);
  /* 65816 interrupt entry sets I and clears decimal mode after stacking P. */
  cpu->_flag_I = 1;
  cpu->_flag_D = 0;
  cpu_mirrors_to_p(cpu);
}

/* Once initialization is complete, Super Tennis sleeps in:
 *
 *   $00:C7B1  LDA $4210
 *   $00:C7B4  BPL $C7B1
 *
 * Mesen shows that regular vblank begins during the LDA: the completed read
 * sees RDNMI=$C2 and the interrupt frame resumes at $C7B4. The frame-model
 * host advances a quiescent CPU directly to the event edge, so reconcile the
 * architectural result of that in-flight poll without charging time again;
 * advance_to_vblank already charged through the edge. The very first NMI is
 * different in the reference run (resume $C7B1, A=$42), hence the count gate. */
static bool reconcile_regular_vblank_poll(void) {
  if (s_nmi_count == 0 || s_resume_pc != kRdnmiPollPc)
    return true;
  /* The final operand fetch for LDA $4210 drives $42 onto the bus before the
   * partially driven RDNMI read. Re-read that ROM byte through the canonical
   * bus so open-bus bits 4-6 match the interrupted instruction. */
  uint8 operand_high = cpu_read8(&g_cpu, 0x00,
                                 (uint16)(kRdnmiPollPc + 2u));
  if (operand_high != 0x42) {
    fprintf(stderr,
            "super-tennis-headless: unexpected RDNMI poll operand $%02X\n",
            operand_high);
    return false;
  }
  uint8 value = cpu_read8(&g_cpu, 0x00, RDNMI);
  cpu_write_a8(&g_cpu, value);
  g_cpu._flag_Z = value == 0;
  g_cpu._flag_N = (value & 0x80) != 0;
  cpu_mirrors_to_p(&g_cpu);
  s_resume_pc = kRdnmiPollPc + 3u;
  return true;
}

static bool begin_nmi(bool reconcile_poll) {
  if (reconcile_poll && !reconcile_regular_vblank_poll())
    return false;
  if (getenv("SNESRECOMP_ST_NMI_TRACE")) {
    fprintf(stderr,
            "[st-nmi] frame=%u nmi=%u return=$%06X master=%llu "
            "A=%04X X=%04X Y=%04X S=%04X D=%04X DB=%02X P=%02X "
            "M%uX%u beam=%u:%u\n",
            s_frame_count, s_nmi_count + 1u, s_resume_pc,
            (unsigned long long)g_cpu.master_cycles, g_cpu.A, g_cpu.X,
            g_cpu.Y, g_cpu.S, g_cpu.D, g_cpu.DB, g_cpu.P,
            g_cpu.m_flag, g_cpu.x_flag, g_snes->vPos, g_snes->hPos);
  }
  prepare_interrupt(&g_cpu, s_resume_pc);
  ++s_nmi_count;
  if (s_active_nmi_depth >=
      sizeof(s_active_nmis) / sizeof(s_active_nmis[0])) {
    fprintf(stderr, "super-tennis: active NMI stack overflow\n");
    return false;
  }
  s_active_nmis[s_active_nmi_depth++] = s_nmi_count;
  s_last_nmi_s = g_cpu.S;
  s_last_nmi_a = g_cpu.A;
  s_last_nmi_y = g_cpu.Y;
  s_last_nmi_p = g_cpu.P;
  s_last_nmi_master = g_cpu.master_cycles;
  if (!write_first_nmi_snapshot() || !capture_entry_if_requested())
    return false;
  /* Leave the handler pending. Whole-program execution runs it through RTI,
   * including a naturally nested NMI if another VBlank arrives first. */
  s_resume_pc = kNmiPc;
  return true;
}

static bool deliver_irq(void) {
  if (!g_snes->inIrq || g_cpu._flag_I)
    return true;
  prepare_interrupt(&g_cpu, s_resume_pc);
  snes_beam_hold(1);
  s_atomic_irq_active = true;
  int ok = interp_bridge_run_interrupt(&g_cpu, kIrqPc);
  s_atomic_irq_active = false;
  snes_beam_hold(0);
  snes_sync_master_clock(g_snes, g_cpu.master_cycles);
  if (!ok) {
    fprintf(stderr,
            "super-tennis: IRQ handler failed at frame %u beam=%u:%u "
            "resume=$%06X S=$%04X\n",
            s_frame_count, g_snes->vPos, g_snes->hPos, s_resume_pc,
            g_cpu.S);
  }
  return ok != 0;
}

/* Move a sleeping/spinning CPU to the next vblank edge. Raster IRQs are
 * sliced and delivered on the way, so they cannot be coalesced by a whole
 * field-sized clock jump. */
static bool advance_to_vblank(void) {
  for (unsigned guard = 0; guard < 1024; ++guard) {
    uint32 to_vblank = snes_master_clocks_until_line(g_snes, kVblankLine);
    if (g_snes->vPos == kVblankLine && g_snes->hPos == 0)
      return true;

    uint32 step = to_vblank;
    uint32 to_irq = snes_master_clocks_until_irq(g_snes);
    if (to_irq && to_irq < step)
      step = to_irq;
    if (!step)
      break;

    /* HDMA runs while snes_sync_master_clock advances the beam and charges
     * its bus time through the master-clock hook. That can carry the final
     * beam position a few scanlines beyond the requested vblank edge. The
     * edge was still crossed: remember that this slice selected vblank (and
     * not an earlier raster IRQ) instead of requiring the final coordinate to
     * remain exactly at line 225, H=0. */
    bool reaches_vblank = step == to_vblank;

    g_cpu.master_cycles += step;
    snes_sync_master_clock(g_snes, g_cpu.master_cycles);
    /* Advancing a sleeping CPU does not displace executable CPU work. Keep
     * the shared DRAM-refresh watermark aligned so the first interrupt
     * instruction does not pay refresh tax for this parked interval. */
    snes_refresh_exempt();
    if (!deliver_irq())
      return false;
    if (reaches_vblank)
      return true;
  }
  fprintf(stderr,
          "super-tennis: advance-to-vblank stalled at frame %u beam=%u:%u "
          "resume=$%06X irq=%u I=%u hirq=%u virq=%u htimer=%u vtimer=%u "
          "master=%llu\n",
          s_frame_count, g_snes->vPos, g_snes->hPos, s_resume_pc,
          g_snes->inIrq ? 1u : 0u, g_cpu._flag_I ? 1u : 0u,
          g_snes->hIrqEnabled ? 1u : 0u, g_snes->vIrqEnabled ? 1u : 0u,
          g_snes->hTimer, g_snes->vTimer,
          (unsigned long long)g_cpu.master_cycles);
  return false;
}

static bool run_mainline_to_wait(uint64 frame_deadline) {
  for (unsigned guard = 0; guard < 1024; ++guard) {
    if (snes_take_nmi(g_snes) && !begin_nmi(false))
      return false;
    uint64 deadline = frame_deadline;
    InterpDeadlineEvent deadline_event = g_snes->nmiEnabled
                                             ? INTERP_DEADLINE_EVENT_NMI
                                             : INTERP_DEADLINE_EVENT_VBLANK;
    uint64 irq_deadline = 0;
    if (snes_next_irq_master(g_snes, g_cpu.master_cycles, &irq_deadline) &&
        irq_deadline < deadline) {
      deadline = irq_deadline;
      deadline_event = INTERP_DEADLINE_EVENT_IRQ;
    }

    interp_bridge_set_master_deadline_event(deadline, deadline_event);
    int ok = interp_bridge_run_until_quiescent(&g_cpu, s_resume_pc);
    interp_bridge_set_master_deadline(0);
    s_resume_pc = interp_bridge_lle_resume_pc();
    if (!ok) {
      fprintf(stderr,
              "super-tennis: mainline bridge failed at frame %u "
              "resume=$%06X beam=%u:%u master=%llu\n",
              s_frame_count, s_resume_pc, g_snes->vPos, g_snes->hPos,
              (unsigned long long)g_cpu.master_cycles);
      return false;
    }
    /* A $4200 0->1 transition can request NMI inside the slice. Give it
     * priority over IRQ and run the handler from the next instruction edge. */
    if (g_snes->nmiPending)
      continue;
    if (!deliver_irq())
      return false;
    if (g_cpu.master_cycles >= frame_deadline)
      s_vblank_due = true;
    if (s_vblank_due)
      return true;
    if (!g_snes->inIrq)
      return true; /* deterministic spin/read cycle or WAI */
  }
  fprintf(stderr,
          "super-tennis: mainline scheduler guard exhausted at frame %u "
          "beam=%u:%u resume=$%06X irq=%u I=%u hirq=%u virq=%u "
          "htimer=%u vtimer=%u master=%llu deadline=%llu\n",
          s_frame_count, g_snes->vPos, g_snes->hPos, s_resume_pc,
          g_snes->inIrq ? 1u : 0u, g_cpu._flag_I ? 1u : 0u,
          g_snes->hIrqEnabled ? 1u : 0u, g_snes->vIrqEnabled ? 1u : 0u,
          g_snes->hTimer, g_snes->vTimer,
          (unsigned long long)g_cpu.master_cycles,
          (unsigned long long)frame_deadline);
  return false;
}

static bool boot_to_wait(void) {
  cpu_state_init(&g_cpu, g_ram);
  interp_bridge_set_post_rti_hook(capture_interrupt_return);
  s_resume_pc = kResetPc;
  s_booted = true;
  return true;
}

static void run_one_frame(void) {
  if (!s_ok)
    return;
  if (!s_booted && !boot_to_wait()) {
    s_ok = false;
    return;
  }
  ++s_frame_count;
  /* Record display-state writes while this field runs. The desktop renderer
   * consumes them per scanline after the CPU reaches the next vblank edge. */
  ppu_rasterBegin(g_ppu);
  /* One call owns exactly one vblank edge. Run productive CPU work toward the
   * edge, or (after a stable hardware-poll cycle/WAI) advance the sleeping
   * machine to it. A deadline yield may land a few master clocks past H=0 at
   * the next architectural instruction boundary; s_vblank_due preserves that
   * already-crossed edge so advance_to_vblank does not wrap another field. */
  {
    uint32 to_vblank = snes_master_clocks_until_line(g_snes, kVblankLine);
    if (!to_vblank)
      to_vblank = kMasterClocksPerField;
    s_vblank_due = false;
    if (!run_mainline_to_wait(g_cpu.master_cycles + to_vblank) || !s_ok) {
      s_ok = false;
      return;
    }
    if (!s_vblank_due && !advance_to_vblank()) {
      s_ok = false;
      return;
    }
    s_vblank_due = false;
  }

  ppu_checkOverscan(g_ppu);
  ppu_handleVblank(g_ppu);
  g_snes->inVblank = true;
  snes_latch_nmi(g_snes);
  if (g_snes->autoJoyRead)
    g_snes->autoJoyTimer = 4224;

  if (snes_take_nmi(g_snes) && !begin_nmi(true)) {
    s_ok = false;
    return;
  }

  /* The next host call resumes at the NMI vector, or at the interrupted
   * continuation when NMI is disabled. */
}

int SuperTennisLastFrameOk(void) { return s_ok ? 1 : 0; }

void SuperTennisDrawPpuFrame(void) {
  ppu_rasterRenderBegin(g_ppu);
  for (int line = 0; line <= 224; ++line) {
    ppu_rasterApplyLine(g_ppu, line);
    ppu_runLine(g_ppu, line);
  }
}

unsigned SuperTennisResumePc(void) { return s_resume_pc; }
unsigned SuperTennisNmiCount(void) { return s_nmi_count; }
unsigned SuperTennisLastNmiStack(void) { return s_last_nmi_s; }
unsigned SuperTennisLastNmiA(void) { return s_last_nmi_a; }
unsigned SuperTennisLastNmiY(void) { return s_last_nmi_y; }
unsigned SuperTennisLastNmiP(void) { return s_last_nmi_p; }
unsigned long long SuperTennisLastNmiMaster(void) {
  return (unsigned long long)s_last_nmi_master;
}

const RtlGameInfo kSuperTennisGameInfo = {
    .title = "super-tennis",
    .initialize = NULL,
    .run_frame = run_one_frame,
    .draw_ppu_frame = SuperTennisDrawPpuFrame,
    .save_name_prefix = "super-tennis",
    .tier2_capture = 0,
};
