"""Private, fail-closed source overlays. Never edit the accepted source tree."""
import re

FRAME_HEADER = 'frame,input,pc,m,x,sp,vpos,hpos,master,nmis,wram,wram_upper,vram,cgram,oam,high_oam,apuram,pixels'


def once(text, old, new):
    if text.count(old) != 1:
        raise ValueError(f'instrumentation anchor changed: {old[:90]!r}')
    return text.replace(old, new, 1)


def host(text):
    text = once(text, 'static void usage(', '#include "frame_checks.h"\n\nstatic void usage(')
    text = once(text, '  if (frame_ppm_path) {', '''  const char *digest_path = getenv("ST_FRAME_DIGESTS");
  FILE *digest_file = digest_path ? fopen(digest_path, "wx") : NULL;
  if (digest_path && !digest_file) Die("cannot create frame digests");
  if (digest_file) {
    setvbuf(digest_file, NULL, _IOLBF, 0);
    fputs("''' + FRAME_HEADER + '''\\n", digest_file);
  }
  if (frame_ppm_path || digest_file) {''')
    text = once(text, '    if (frame_ppm_path)\n      SuperTennisDrawPpuFrame();', '''    if (frame_ppm_path || digest_file)
      SuperTennisDrawPpuFrame();
    if (digest_file) validation_frame(digest_file, frame_number, input);''')
    return once(text, '  if (audio_events_path &&', '  if (digest_file && fclose(digest_file)) return 6;\n  if (audio_events_path &&')


def interpreter(text):
    text = once(text, 'int interp816_runOpcode(', '#include "work_profile.h"\n\nint interp816_runOpcode(')
    text = once(text, '  uint8_t opcode = interp816_readOpcode(cpu);',
                '  unsigned wp_modes = (!!cpu->mf << 1) | !!cpu->xf;\n  uint8_t opcode = interp816_readOpcode(cpu);')
    return once(text, '  interp816_doOpcode(cpu, opcode);',
                '  interp816_doOpcode(cpu, opcode);\n  wp_record(_pcb, wp_modes, opcode, cpu->cyclesUsed);')


def bridge(text, trace):
    text = once(text, 'int g_aot_instruction_read_active;', '#include "diagnostics.h"\nint g_aot_instruction_read_active;')
    text = once(text, '            RecompReturn result = continuation->body(cpu);', '''            uint64_t st_before = cpu->cycles;
            unsigned st_m = in.mf, st_x = in.xf;
            RecompReturn result = continuation->body(cpu);
            if (cpu->cycles > st_before) st_record_entry(pc_before, st_m, st_x);''')
    if trace:
        text = once(text, '        int _cyc = interp816_runOpcode(&in);', '''        /* Copy current interpreter registers; shared cpu may be stale. */
        if (!in.stopped && !in.waiting && !in.nmiWanted && (in.i || !in.irqWanted)) {
            CpuState st_snapshot = *cpu;
            sync_interp_to_cpu(&in, &st_snapshot);
            st_trace_before(&st_snapshot, pc_before);
        }
        int _cyc = interp816_runOpcode(&in);''')
    return text


def generated(text, roots, trace):
    found = set()
    trace_sites = 0
    pattern = r'(?m)^([ \t]*)(_aot_timing = \(CpuAotInstructionTiming\)\{[^\n]+\};\n[ \t]*cpu_aot_insn_bus_extra\(&_aot_timing, (\d+), 0x([0-9A-Fa-f]+), \d+\);)'
    addresses = {int(key.split(':')[0], 16) for key in roots}
    def hook(match):
        nonlocal trace_sites
        pc = int(match[3]) * 65536 + int(match[4], 16)
        lines = []
        if pc in addresses:
            found.add(pc)
            lines.append(f'st_record_entry(0x{pc:06X}u, cpu->m_flag, cpu->x_flag);')
        if trace:
            trace_sites += 1
            lines.append(f'st_trace_before(cpu, 0x{pc:06X}u);')
        return ''.join(match[1] + line + '\n' for line in lines) + match[0]
    text = re.sub(pattern, hook, text)
    if found or trace_sites:
        prototypes = '\nextern void st_record_entry(unsigned, unsigned, unsigned);\nextern void st_trace_before(CpuState *, unsigned);\n'
        text = once(text, '#include "funcs.h"', '#include "funcs.h"' + prototypes)
    return text, found, trace_sites


def host_cost(text):
    """Attribute bridge work to exact keys, excluding native callees.

    Cleanup restores nested interpreter ownership on every return or continue.
    Outside-bridge time includes rendering. Device completion stays with the
    owning instruction, so group cost is an upper bound on removable overhead.
    """
    text = once(text, 'int g_aot_instruction_read_active;', '#include "host_cost.h"\nint g_aot_instruction_read_active;')
    text = once(text, '    const int auto_quiescent = yield_pc == 0xFFFFFFFEu;', '''    hp_init();
    HpScope hp_owner __attribute__((cleanup(hp_restore))) = {hp_current};
    hp_set(1);
    const int auto_quiescent = yield_pc == 0xFFFFFFFEu;''')
    text = once(text, '        const uint32_t pc_before = ((uint32_t)in.k << 16) | in.pc;', '''        const uint32_t pc_before = ((uint32_t)in.k << 16) | in.pc;
        hp_set(hp_group((pc_before << 2) | (!!in.mf << 1) | !!in.xf));
        hp_visits[hp_current]++;''')
    for expression in ('RecompReturn result = continuation->body(cpu);',
                       'RecompReturn _air = cpu_dispatch_pc_paired(cpu, target, _fs);'):
        text = once(text, expression, 'unsigned hp_before_native = hp_current;\n'
                    '            hp_set(2);\n            ' + expression + '\n'
                    '            hp_set(hp_before_native);')
    return text
