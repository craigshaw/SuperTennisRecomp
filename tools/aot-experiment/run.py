#!/usr/bin/env python3
"""Bounded private AOT experiments: lock inputs, generate, build, replay, compare."""
import argparse
from collections import deque
import csv
import hashlib
import importlib.util
import itertools
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

from instrument import FRAME_HEADER, bridge, generated, host, interpreter, host_cost

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
CAPTURES = ROOT / 'captures'
ROM_SHA = '6e45a80ea148654514cb4e8604a0ffcbc726946e70f9e0b9860e36c0f3fa4877'
KEY = re.compile(r'[0-9A-F]{6}:[01]:[01]')


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def private(path):
    path = Path(path).resolve()
    if not path.is_relative_to(CAPTURES.resolve()) or path == CAPTURES.resolve():
        raise ValueError('experiment outputs must be below ignored captures/')
    return path


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def tree(path, patterns=('*',)):
    path = Path(path)
    files = sorted({p for pattern in patterns for p in path.rglob(pattern) if p.is_file()})
    return {str(p.relative_to(path)): sha(p) for p in files}


def locked(path, directory=False):
    path = Path(path).resolve()
    return {'path': str(path), 'sha256': tree(path) if directory else sha(path)}


def clean_env():
    return {k: v for k, v in os.environ.items() if not k.startswith(('ST_', 'SNESRECOMP_'))}


def compare(left, right, rows=None):
    """Compare ordered records. A requested frame prefix must exist on both sides."""
    previous = deque(maxlen=3)
    with Path(left).open() as a, Path(right).open() as b:
        aa, bb = csv.DictReader(a), csv.DictReader(b)
        if aa.fieldnames != bb.fieldnames:
            return {'equal': False, 'reason': 'header', 'left': aa.fieldnames, 'right': bb.fieldnames}
        pairs = itertools.zip_longest(aa, bb)
        if rows is not None:
            pairs = itertools.islice(pairs, rows)
        count = 0
        for count, (x, y) in enumerate(pairs, 1):
            if x != y:
                fields = [k for k in (x or y) if x is None or y is None or x[k] != y[k]]
                return {'equal': False, 'row': count, 'fields': fields,
                        'left': x, 'right': y, 'previous': list(previous)}
            previous.append(x)
        return {'equal': count > 0 and (rows is None or count == rows), 'rows': count,
                'scope': 'requested prefix' if rows is not None else 'entire files'}


def preflight(config):
    if config.get('version') != 1:
        raise ValueError('unsupported experiment config version')
    for name, item in config['inputs'].items():
        path = Path(item['path'])
        actual = tree(path) if isinstance(item['sha256'], dict) else sha(path)
        if actual != item['sha256']:
            raise ValueError(f'locked input changed: {name}; make a new config deliberately')
    if not {'rom', 'replay', 'frames', 'cfg'} <= config['inputs'].keys():
        raise ValueError('required input missing')
    if 'generated' in config['inputs'] and config.get('selection'):
        raise ValueError('selection overrides cannot be applied to frozen generated output')
    if config['inputs']['rom']['sha256'] != ROM_SHA:
        raise ValueError('unsupported ROM SHA-256')
    if not 0 < config['short_frames'] <= config['full_frames']:
        raise ValueError('require 0 < short_frames <= full_frames')
    for key in config['roots'] + config['require_executed']:
        if not KEY.fullmatch(key):
            raise ValueError(f'exact key required: {key}')
    replay_bytes = Path(config['inputs']['replay']['path']).read_bytes()
    if (len(replay_bytes) < 232 or replay_bytes[:8] != b'SNRPLY\x1a\n' or
            replay_bytes[36:68].hex() != ROM_SHA or (len(replay_bytes) - 232) % 24 or
            (len(replay_bytes) - 232) // 24 < config['full_frames']):
        raise ValueError('replay identity, size or coverage invalid; runtime checks the full protocol')
    reference = config['inputs']['frames']['path']
    with open(reference) as f:
        reader = csv.DictReader(f)
        if reader.fieldnames != FRAME_HEADER.split(','):
            raise ValueError('unsupported frame reference schema')
        n = 0
        for n, row in enumerate(reader, 1):
            if int(row['frame']) != n or None in row or None in row.values():
                raise ValueError('invalid reference frame sequence')
        if n < config['full_frames']:
            raise ValueError('frame control shorter than requested replay')
    trace = config.get('trace')
    if trace:
        if not 0 <= trace['from'] <= trace['to'] < config['short_frames'] or not 0 < trace['limit'] <= 1000000:
            raise ValueError('invalid bounded trace window or row limit')
        if not 1 <= len(trace['ranges']) <= 32:
            raise ValueError('trace requires 1..32 address ranges')
        for value in trace['ranges']:
            if not re.fullmatch(r'[0-9A-F]{6}-[0-9A-F]{6}', value) or value[:6] > value[7:]:
                raise ValueError('invalid trace range')
    for key, values in config.get('selection', {}).items():
        if key not in ('instruction_timing', 'continuations') or set(values) - {'add', 'remove'}:
            raise ValueError('unsupported selection override')
        for value in values.get('add', []) + values.get('remove', []):
            if not re.fullmatch(r'[0-9A-F]{6}:[01]:[01]' + (r'>[0-9A-F]{6}:[01]:[01]' if key == 'continuations' else ''), value):
                raise ValueError('invalid selection key')
    if not re.fullmatch(r'[a-zA-Z0-9_-]{1,64}', config.get('cache', 'default')):
        raise ValueError('invalid cache name')
    groups = config.get('host_cost', {})
    seen = set()
    if len(groups) > 16:
        raise ValueError('at most 16 host cost groups')
    for name, keys in groups.items():
        if not re.fullmatch(r'[a-z][a-z0-9_]{0,40}', name) or name in ('outside_bridge', 'other_interpreter', 'native_callees'):
            raise ValueError('invalid host cost group name')
        for key in keys:
            if not KEY.fullmatch(key) or key in seen:
                raise ValueError('host cost keys must be exact and disjoint')
            seen.add(key)
    return {'locked_inputs': list(config['inputs']), 'control_frames': n}


def snapshot():
    def git(path, *args):
        return subprocess.check_output(['git', '-C', str(path), *args], text=True).strip()
    paths = ['src', 'config', 'tools/aot-experiment', 'snesrecomp/runner', 'snesrecomp/recompiler', 'snesrecomp/tools']
    return {'title': git(ROOT, 'rev-parse', 'HEAD'),
            'shared': git(ROOT / 'snesrecomp', 'rev-parse', 'HEAD'),
            'title_status': git(ROOT, 'status', '--short'),
            'shared_status': git(ROOT / 'snesrecomp', 'status', '--short'),
            'files': {p: tree(ROOT / p, ('*.c', '*.h', '*.py', '*.cfg', '*.cmake', 'CMakeLists.txt')) for p in paths},
            'generation_policy': sha(ROOT / 'tools/generate-normal.py')}


def command(argv, log, env=None):
    start = time.monotonic()
    with Path(log).open('x') as output:
        result = subprocess.run([str(x) for x in argv], cwd=ROOT, env=env or clean_env(),
                                stdout=output, stderr=subprocess.STDOUT)
    record = {'argv': [str(x) for x in argv], 'code': result.returncode,
              'seconds': round(time.monotonic() - start, 4)}
    save(str(log) + '.json', record)
    if result.returncode:
        raise RuntimeError(f'command failed ({result.returncode}); see {log}')
    return record


def generate(config, out):
    if 'generated' in config['inputs']:
        shutil.copytree(config['inputs']['generated']['path'], out / 'generated')
        return
    spec = importlib.util.spec_from_file_location('normal_policy', ROOT / 'tools/generate-normal.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    env = module.generation_environment(clean_env())
    for key, values in config.get('selection', {}).items():
        name = 'SNESRECOMP_EMIT_' + key.upper()
        selected = env[name].split(',')
        for item in values.get('remove', []):
            selected.remove(item)
        selected += [x for x in values.get('add', []) if x not in selected]
        env[name] = ','.join(selected)
    shutil.copytree(config['inputs']['cfg']['path'], out / 'config')
    save(out / 'generation-options.json', {k: v for k, v in env.items() if k.startswith('SNESRECOMP_')})
    command([sys.executable, ROOT / 'snesrecomp/tools/v2_emit.py', '--rom', config['inputs']['rom']['path'],
             '--cfg-dir', out / 'config', '--out-dir', out / 'generated', '--source-root', ROOT / 'src',
             '--analysis-backend', 'python', '--cfg-roots'], out / 'generate.log', env)


def overlay(config, out, tracing, profiling=False):
    dest = out / 'overlay'
    shutil.copytree(out / 'generated', dest / 'generated')
    roots, sites = set(), 0
    for path in (dest / 'generated').glob('*.c'):
        text, found, count = generated(path.read_text(), [] if profiling else config['roots'], tracing)
        path.write_text(text)
        roots.update(found)
        sites += count
    missing = [key for key in config['roots'] if int(key[:6], 16) not in roots]
    if missing and not profiling:
        raise ValueError(f'roots have no native instruction timing hook: {missing}')
    if tracing and not sites:
        raise ValueError('no native trace anchors found')
    source = ROOT / 'snesrecomp/runner/src/snes'
    interp = (source / 'interp816.c').read_text()
    bridge_text = (source / 'interp_bridge.c').read_text()
    (dest / 'interp816.c').write_text(interp if profiling else interpreter(interp))
    if not profiling:
        bridge_text = bridge(bridge_text, tracing)
    if config.get('host_cost'):
        bridge_text = host_cost(bridge_text)
        names = ['outside_bridge', 'other_interpreter', 'native_callees'] + list(config['host_cost'])
        lines = [f'#define HP_GROUPS {len(names)}',
                 'static const char *hp_names[] = {' + ','.join(json.dumps(n) for n in names) + '};',
                 'static unsigned hp_group(unsigned key) { switch(key) {']
        for i, keys in enumerate(config['host_cost'].values(), 3):
            for key in keys:
                pc, m, x = key.split(':')
                lines.append(f'case {(int(pc,16)<<2)|(int(m)<<1)|int(x)}u: return {i};')
        lines.append('default: return 1; }}')
        (dest / 'host_cost_groups.h').write_text('\n'.join(lines)+'\n')
    (dest / 'interp_bridge.c').write_text(bridge_text)
    (dest / 'headless_main.c').write_text(host((ROOT / 'src/headless_main.c').read_text()))
    for path in HERE.glob('*.h'):
        shutil.copy2(path, dest / path.name)
    return dest


def sync_tree(source, dest):
    """Preserve timestamps for identical files so Ninja reuses its dependency graph."""
    dest.mkdir(parents=True, exist_ok=True)
    expected = tree(source)
    for path in dest.rglob('*'):
        if path.is_file() and str(path.relative_to(dest)) not in expected:
            path.unlink()
    for name, digest in expected.items():
        target = dest / name
        if not target.exists() or sha(target) != digest:
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / name, target)


def build(config, out, source, lane):
    cache = private(CAPTURES / 'aot-experiment-cache' / lane)
    cache.mkdir(parents=True, exist_ok=True)
    # Exclusive lock prevents two experiments changing the same overlay during a build.
    lock = cache / 'busy'
    with lock.open('x'):
        pass
    try:
        sync_tree(source, cache / 'overlay')
        command(['cmake', '-S', HERE, '-B', cache / 'build', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
                 f'-DTITLE_ROOT={ROOT}', f'-DOVERLAY={cache / "overlay"}'], out / 'configure.log')
        result = command(['cmake', '--build', cache / 'build', '--parallel', '4'], out / 'build.log')
        shutil.copy2(cache / 'build/experiment', out / 'experiment')
        result.update(binary_sha256=sha(out / 'experiment'),
                      no_compile='no work to do' in (out / 'build.log').read_text())
        save(out / 'build-result.json', result)
    finally:
        lock.unlink()


def replay(config, out, frames, tracing=False):
    out.mkdir()
    env = clean_env()
    env.update(ST_AOT_ENTRIES=str(out / 'entries.csv'), ST_CPU_WORK_PROFILE=str(out / 'opcode-work.csv'),
               ST_FRAME_DIGESTS=str(out / 'frames.csv'), ST_FRAME_REFERENCE=config['inputs']['frames']['path'],
               SNESRECOMP_TIER2_CAPTURE='1', SNESRECOMP_TIER2_MANIFEST=str(out / 'tier2.json'),
               SNESRECOMP_TIER2_JOURNAL=str(out / 'tier2.jsonl'))
    if config.get('host_cost'):
        env['ST_HOST_COST'] = str(out / 'host-cost.json')
    if tracing:
        t = config['trace']
        env.update(ST_INSTRUCTION_TRACE=str(out / 'trace.csv'), ST_TRACE_FROM=str(t['from']),
                   ST_TRACE_TO=str(t['to']), ST_TRACE_LIMIT=str(t['limit']), ST_TRACE_RANGES=','.join(t['ranges']))
    save(out / 'environment.json', {k: v for k, v in env.items() if k.startswith(('ST_', 'SNESRECOMP_'))})
    error = None
    try:
        command([out.parent / 'experiment', config['inputs']['rom']['path'], '--replay',
                 config['inputs']['replay']['path'], '--frames', str(frames)], out / 'run.log', env)
    except RuntimeError as exc:
        error = str(exc)
    result = {'frames_requested': frames, 'error': error}
    if (out / 'frames.csv').exists():
        result['comparison'] = compare(config['inputs']['frames']['path'], out / 'frames.csv', frames)
    if not error:
        tier = json.loads((out / 'tier2.json').read_text())
        result['diagnostics'] = {'bailouts': sum(d['bail_hits'] for d in tier['discoveries']),
                                 'overflow': tier['overflowed_tuples'], 'journal_failures': tier['journal_write_failures']}
        result['profile'] = json.loads((out / 'opcode-work.csv.summary.json').read_text())
        with (out / 'opcode-work.csv').open() as f:
            result['changed_opcodes'] = sum(int(x['changed_opcode']) for x in csv.DictReader(f))
        with (out / 'entries.csv').open() as f:
            result['entries'] = {r['key']: int(r['entries']) for r in csv.DictReader(f)}
        result['missing_execution'] = [k for k in config['require_executed'] if not result['entries'].get(k)]
        p = result['profile']
        result['passed'] = (result['comparison']['equal'] and not any(result['diagnostics'].values()) and
                            not p['overflow'] and not result['changed_opcodes'] and
                            p['instructions'] == p['existing_counter_instructions'] and
                            p['cpu_cycles'] == p['existing_counter_cpu_cycles'] and
                            not result['missing_execution'])
        if frames == config['full_frames'] and 'profile' in config['inputs']:
            with open(config['inputs']['profile']['path']) as f:
                prior = sum(int(x['instructions']) for x in csv.DictReader(f))
            result['interpreted_instruction_delta'] = p['instructions'] - prior
        if tracing:
            match = re.search(r'trace rows=(\d+) limit=(\d+) truncated=(\d+)', (out / 'run.log').read_text())
            if not match:
                raise ValueError('missing trace completion metadata')
            result['trace'] = dict(zip(('rows', 'limit', 'truncated'), map(int, match.groups())))
            result['trace']['sha256'] = sha(out / 'trace.csv')
            result['passed'] = result['passed'] and result['trace']['rows'] > 0
    save(out / 'result.json', result)
    if not result.get('passed'):
        raise RuntimeError(f'replay validation failed; see {out / "result.json"}')
    return result


def run(args):
    config = json.loads(args.config.read_text())
    checked = preflight(config)
    if args.action == 'preflight':
        print(json.dumps(checked))
        return
    out = private(args.out)
    out.mkdir(parents=True, exist_ok=False)
    save(out / 'config.json', config)
    save(out / 'source-before.json', snapshot())
    try:
        generate(config, out)
        tracing = args.action == 'trace'
        if tracing and not config.get('trace'):
            raise ValueError('trace window missing in config')
        profiling = args.action == 'profile'
        if profiling and args.short_only:
            raise ValueError('profile uses full_frames; use run --short-only for validation')
        if profiling and not config.get('host_cost'):
            raise ValueError('host_cost groups missing in config')
        source = overlay(config, out, tracing, profiling)
        lane = config.get('cache', 'default') + ('-profile' if profiling else '-trace' if tracing else '-measure')
        build(config, out, source, lane)
        if profiling:
            env = clean_env()
            env['ST_HOST_COST'] = str(out / 'host-cost.json')
            command([out / 'experiment', config['inputs']['rom']['path'], '--replay',
                     config['inputs']['replay']['path'], '--frames', str(config['full_frames']),
                     '--frame-ppm', out / 'final.ppm'], out / 'run.log', env)
            cost = json.loads((out / 'host-cost.json').read_text())
            if (cost['wall_seconds'] <= 0 or
                    abs(sum(g['elapsed_ns'] for g in cost['groups']) / 1e9 - cost['wall_seconds']) > 0.000001):
                raise ValueError('host cost scopes do not account for elapsed time')
            missing_groups = [g['name'] for g in cost['groups']
                              if g['name'] in config['host_cost'] and not g['visits']]
            if missing_groups:
                raise ValueError(f'host cost groups did not execute: {missing_groups}')
            result = {'passed': True, 'host_cost': cost,
                      'final_pixels_sha256': sha(out / 'final.ppm')}
        else:
            result = replay(config, out / 'short', config['short_frames'], tracing)
            if not args.short_only and not tracing and config['full_frames'] > config['short_frames']:
                result = replay(config, out / 'full', config['full_frames'])
        preflight(config)  # Input changes during a run invalidate the result too.
        after = snapshot()
        save(out / 'source-after.json', after)
        if after != json.loads((out / 'source-before.json').read_text()):
            raise RuntimeError('source changed during experiment; results cannot be accepted')
        result['scope'] = 'host elapsed attribution; no frame validation' if profiling else 'bounded trace' if tracing else ('short probe' if args.short_only else 'representative replay')
        save(out / 'result.json', result)
        print(json.dumps({'passed': True, 'scope': result['scope'], 'evidence': str(out),
                          'instructions': result.get('profile', {}).get('instructions'), 'entries': result.get('entries')}))
    except Exception as exc:
        if not (out / 'result.json').exists():
            save(out / 'result.json', {'passed': False, 'error': str(exc)})
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    init = sub.add_parser('init', help='write an exclusive private input lock')
    for name in ('rom', 'replay', 'frames', 'cfg'):
        init.add_argument('--' + name, type=Path, required=True)
    init.add_argument('--profile', type=Path)
    init.add_argument('--generated', type=Path, help='reuse frozen generated output instead of generating')
    init.add_argument('--root', action='append', default=[])
    init.add_argument('--require-executed', action='append', default=[])
    init.add_argument('--short-frames', type=int, required=True)
    init.add_argument('--full-frames', type=int, required=True)
    init.add_argument('--out', type=Path, required=True)
    for action in ('run', 'trace', 'preflight', 'profile'):
        p = sub.add_parser(action)
        p.add_argument('config', type=Path)
        if action != 'preflight':
            p.add_argument('--out', type=Path, required=True)
            p.add_argument('--short-only', action='store_true')
    cmp = sub.add_parser('compare', help='first differing CSV record, with three preceding records')
    cmp.add_argument('left', type=Path)
    cmp.add_argument('right', type=Path)
    cmp.add_argument('--rows', type=int)
    args = parser.parse_args()
    try:
        if args.action == 'init':
            inputs = {name: locked(getattr(args, name), name in ('cfg', 'generated'))
                      for name in ('rom', 'replay', 'frames', 'cfg', 'profile', 'generated') if getattr(args, name)}
            config = {'version': 1, 'inputs': inputs, 'roots': args.root,
                      'require_executed': args.require_executed, 'short_frames': args.short_frames,
                      'full_frames': args.full_frames, 'selection': {}}
            preflight(config)
            path = private(args.out)
            path.parent.mkdir(parents=True, exist_ok=True)
            save(path, config)
            print(path)
        elif args.action == 'compare':
            result = compare(args.left, args.right, args.rows)
            # Trace files carry completion metadata beside them; never call a capped match complete.
            for side in ('left', 'right'):
                meta = getattr(args, side).parent / 'result.json'
                if meta.exists():
                    trace = json.loads(meta.read_text()).get('trace')
                    if trace:
                        if trace.get('sha256') and trace['sha256'] != sha(getattr(args, side)):
                            raise ValueError('trace content differs from its completion metadata')
                        result[side + '_trace'] = trace
                        if trace['truncated']:
                            result['scope'] = 'capped trace prefix only'
            print(json.dumps(result, indent=2))
            return 0 if result['equal'] else 1
        else:
            run(args)
    except (ValueError, RuntimeError, OSError, KeyError) as exc:
        print(f'aot-experiment: {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
