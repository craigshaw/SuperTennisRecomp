/* Private elapsed-time attribution for the single-threaded headless host.
 * Includes device completion and measurement overhead. Not predicted savings. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "host_cost_groups.h"

static unsigned hp_current;
static uint64_t hp_visits[HP_GROUPS], hp_elapsed[HP_GROUPS], hp_changes[HP_GROUPS];
static uint64_t hp_last_tick, hp_start_tick;
static int hp_started;
static struct rusage hp_start_usage;
static uint64_t hp_clock(void) {
  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now)) _Exit(6);
  return (uint64_t)now.tv_sec * 1000000000u + now.tv_nsec;
}
#ifndef HP_CLOCK
#define HP_CLOCK hp_clock
#endif
static void hp_set(unsigned group) {
  if (group == hp_current) return;
  uint64_t now = HP_CLOCK();
  hp_elapsed[hp_current] += now - hp_last_tick;
  hp_changes[hp_current]++;
  hp_last_tick = now;
  hp_current = group;
}
typedef struct { unsigned previous; } HpScope;
static void hp_restore(HpScope *scope) { hp_set(scope->previous); }
static double hp_seconds(struct timeval t) { return t.tv_sec + t.tv_usec / 1000000.0; }
static void hp_dump(void) {
  uint64_t now = HP_CLOCK();
  hp_elapsed[hp_current] += now - hp_last_tick;
  struct rusage end;
  if (getrusage(RUSAGE_SELF, &end)) _Exit(6);
  FILE *f = fopen(getenv("ST_HOST_COST"), "wx");
  if (!f) _Exit(6);
  fprintf(f,"{\"user_seconds\":%.9f,\"system_seconds\":%.9f,\"wall_seconds\":%.9f,\"groups\":[",
    hp_seconds(end.ru_utime)-hp_seconds(hp_start_usage.ru_utime),
    hp_seconds(end.ru_stime)-hp_seconds(hp_start_usage.ru_stime),
    (now-hp_start_tick)/1000000000.0);
  for (unsigned i=0;i<HP_GROUPS;i++)
    fprintf(f,"%s{\"name\":\"%s\",\"visits\":%llu,\"elapsed_ns\":%llu,\"transitions\":%llu}",
      i?",":"",hp_names[i],(unsigned long long)hp_visits[i],
      (unsigned long long)hp_elapsed[i],(unsigned long long)hp_changes[i]);
  fputs("]}\n",f); if (fclose(f)) _Exit(6);
}
static void hp_init(void) {
  if (hp_started) return;
  hp_started=1;
  hp_start_tick=hp_last_tick=HP_CLOCK();
  if (!getenv("ST_HOST_COST")) return;
  if (getrusage(RUSAGE_SELF,&hp_start_usage)) _Exit(6);
  atexit(hp_dump);
}
