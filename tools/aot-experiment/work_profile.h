/* Private measurement only. Never linked into the accepted build. */
#include <stdio.h>
#include <inttypes.h>
extern int snes_frame_counter;
uint64_t interp816_insns_total(void);
uint64_t interp816_cycles_total(void);
#define WP_SLOTS 65536u
#define WP_BINS 9u
struct WorkRow {
  uint32_t key, first_frame, last_frame, opcode;
  uint64_t changed_opcode, insns[WP_BINS], cycles[WP_BINS];
};
static struct WorkRow wp_rows[WP_SLOTS];
static uint64_t wp_overflow;
static void wp_dump(void);
static void wp_record(uint32_t pc, unsigned modes, unsigned opcode, unsigned cycles) {
  static int initialized;
  if (!initialized) { atexit(wp_dump); initialized = 1; }
  uint32_t key = ((pc & 0xffffffu) << 2 | modes) + 1u;
  unsigned slot = (key * 2654435761u) & (WP_SLOTS - 1u);
  unsigned frame = snes_frame_counter < 0 ? 0u : (unsigned)snes_frame_counter;
  unsigned bin = frame / 600u;
  if (bin >= WP_BINS) bin = WP_BINS - 1u;
  for (unsigned probe = 0; probe < WP_SLOTS; ++probe) {
    struct WorkRow *row = &wp_rows[slot];
    if (!row->key) {
      row->key = key; row->first_frame = frame; row->opcode = opcode;
    }
    if (row->key == key) {
      row->last_frame = frame;
      row->changed_opcode += row->opcode != opcode;
      row->insns[bin]++; row->cycles[bin] += cycles;
      return;
    }
    slot = (slot + 1u) & (WP_SLOTS - 1u);
  }
  wp_overflow++;
}
static void wp_dump(void) {
  const char *path = getenv("ST_CPU_WORK_PROFILE");
  if (!path) return;
  FILE *f = fopen(path, "wx");
  if (!f) { perror("work profile"); _Exit(6); }
  fputs("pc,m,x,opcode,changed_opcode,instructions,cpu_cycles,first_frame,last_frame", f);
  for (unsigned b=0;b<WP_BINS;b++) fprintf(f,",bin%u_instructions,bin%u_cpu_cycles",b,b);
  fputc('\n',f);
  uint64_t total_i=0,total_c=0; unsigned unique=0;
  for (unsigned i=0;i<WP_SLOTS;i++) {
    struct WorkRow *row=&wp_rows[i]; if (!row->key) continue;
    uint32_t key=row->key-1; uint64_t ni=0,nc=0;
    for(unsigned b=0;b<WP_BINS;b++){ni+=row->insns[b];nc+=row->cycles[b];}
    fprintf(f,"%06X,%u,%u,%02X,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,%u",
      key>>2,(key>>1)&1,key&1,row->opcode,row->changed_opcode,ni,nc,row->first_frame,row->last_frame);
    for(unsigned b=0;b<WP_BINS;b++)fprintf(f,",%" PRIu64 ",%" PRIu64,row->insns[b],row->cycles[b]);
    fputc('\n',f);total_i+=ni;total_c+=nc;unique++;
  }
  fclose(f);
  char summary[4096]; snprintf(summary,sizeof summary,"%s.summary.json",path);
  f=fopen(summary,"wx"); if(!f) _Exit(6);
  fprintf(f,"{\"unique_keys\":%u,\"overflow\":%" PRIu64 ",\"instructions\":%" PRIu64 ",\"cpu_cycles\":%" PRIu64 ",\"existing_counter_instructions\":%" PRIu64 ",\"existing_counter_cpu_cycles\":%" PRIu64 "}\n",
    unique,wp_overflow,total_i,total_c,interp816_insns_total(),interp816_cycles_total());
  fclose(f);
}
