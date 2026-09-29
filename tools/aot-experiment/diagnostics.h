/* Included in a private copy of interp_bridge.c, after its APU accumulator.
 * Observe architecture through a snapshot. Never mutate live P or read MMIO. */
#include "apu.h"
#include <inttypes.h>

typedef struct StEntry { unsigned key; uint64_t hits; } StEntry;
static StEntry st_entries[4096];
static FILE *st_trace_file;
static unsigned st_trace_count, st_trace_limit, st_trace_truncated;
static unsigned st_trace_from, st_trace_to, st_trace_ranges[32][2], st_range_count;
static int st_diag_initialized;

static void st_diag_dump(void) {
  const char *path = getenv("ST_AOT_ENTRIES");
  if (path) {
    FILE *f = fopen(path, "wx");
    if (!f) _Exit(6);
    fputs("key,entries\n", f);
    for (unsigned i=0; i<4096; i++) if (st_entries[i].key) {
      unsigned k=st_entries[i].key-1;
      fprintf(f,"%06X:%u:%u,%" PRIu64 "\n",k>>2,(k>>1)&1,k&1,st_entries[i].hits);
    }
    if (fclose(f)) _Exit(6);
  }
  if (st_trace_file) {
    if (fclose(st_trace_file)) _Exit(6);
    fprintf(stderr,"trace rows=%u limit=%u truncated=%u\n",
            st_trace_count,st_trace_limit,st_trace_truncated);
  }
}
static void st_diag_init(void) {
  if (st_diag_initialized) return;
  st_diag_initialized=1;
  atexit(st_diag_dump);
  const char *path=getenv("ST_INSTRUCTION_TRACE");
  if (!path) return;
  st_trace_file=fopen(path,"wx");
  if (!st_trace_file) _Exit(6);
  st_trace_from=(unsigned)strtoul(getenv("ST_TRACE_FROM"),NULL,10);
  st_trace_to=(unsigned)strtoul(getenv("ST_TRACE_TO"),NULL,10);
  st_trace_limit=(unsigned)strtoul(getenv("ST_TRACE_LIMIT"),NULL,10);
  const char *ranges=getenv("ST_TRACE_RANGES");
  while (ranges && *ranges && st_range_count<32) {
    unsigned lo,hi; int n=0;
    if (sscanf(ranges,"%x-%x%n",&lo,&hi,&n)!=2) _Exit(6);
    st_trace_ranges[st_range_count][0]=lo;
    st_trace_ranges[st_range_count++][1]=hi;
    ranges+=n; if (*ranges==',') ranges++;
  }
  fputs("frame,pc,m,x,a,xreg,y,sp,d,db,pb,p,cycles,master,apu_clock,apu_target,apu_pending,apu_credit,in_ports,out_ports\n",st_trace_file);
}
void st_record_entry(unsigned pc,unsigned m,unsigned x) {
  st_diag_init();
  unsigned key=(pc<<2 | m<<1 | x)+1,slot=(key*2654435761u)&4095;
  for (unsigned i=0;i<4096;i++,slot=(slot+1)&4095) {
    if (!st_entries[slot].key) st_entries[slot].key=key;
    if (st_entries[slot].key==key) { st_entries[slot].hits++; return; }
  }
  fputs("entry counter overflow\n",stderr); _Exit(6);
}
void st_trace_before(CpuState *cpu,unsigned pc) {
  st_diag_init();
  if (!st_trace_file || snes_frame_counter<(int)st_trace_from ||
      snes_frame_counter>(int)st_trace_to) return;
  unsigned match=!st_range_count;
  for (unsigned i=0;i<st_range_count;i++)
    match |= pc>=st_trace_ranges[i][0] && pc<=st_trace_ranges[i][1];
  if (!match) return;
  if (st_trace_count>=st_trace_limit) { st_trace_truncated=1; return; }
  st_trace_count++;
  CpuState snapshot=*cpu;
  cpu_mirrors_to_p(&snapshot);
  Apu *apu=g_snes->apu;
  fprintf(st_trace_file,"%d,%06X,%u,%u,%04X,%04X,%04X,%04X,%04X,%02X,%02X,%02X,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.17g,%02X%02X%02X%02X,%02X%02X%02X%02X\n",
    snes_frame_counter,pc,snapshot.m_flag,snapshot.x_flag,snapshot.A,snapshot.X,
    snapshot.Y,snapshot.S,snapshot.D,snapshot.DB,snapshot.PB,snapshot.P,
    snapshot.cycles,snapshot.master_cycles,apu->portClock,apu->portLastTarget,
    s_apu_pending_master,g_snes->apuCatchupCycles,
    apu->inPorts[0],apu->inPorts[1],apu->inPorts[2],apu->inPorts[3],
    apu->outPorts[0],apu->outPorts[1],apu->outPorts[2],apu->outPorts[3]);
}
