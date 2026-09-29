"""Failure and reuse contracts. No ROM or shared compiler suite required."""
import json
from pathlib import Path
import tempfile
import unittest

import instrument
import run


class ExperimentTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def csv_file(self, name, content):
        p = self.root / name
        p.write_text(content)
        return p

    def test_first_difference_and_context(self):
        a = self.csv_file('a.csv', 'frame,pc\n1,ABCD\n2,BCDE\n3,CDEF\n')
        b = self.csv_file('b.csv', 'frame,pc\n1,ABCD\n2,FFFF\n3,CDEF\n')
        result = run.compare(a, b)
        self.assertFalse(result['equal'])
        self.assertEqual((result['row'], result['fields']), (2, ['pc']))
        self.assertEqual(len(result['previous']), 1)

    def test_truncated_missing_and_empty_inputs_fail(self):
        a = self.csv_file('a.csv', 'frame,pc\n1,ABCD\n2,BCDE\n')
        b = self.csv_file('b.csv', 'frame,pc\n1,ABCD\n')
        empty = self.csv_file('empty.csv', 'frame,pc\n')
        self.assertFalse(run.compare(a, b)['equal'])
        self.assertFalse(run.compare(a, b, 2)['equal'])
        self.assertFalse(run.compare(b, b, 2)['equal'])
        self.assertFalse(run.compare(empty, empty)['equal'])
        self.assertTrue(run.compare(a, b, 1)['equal'])

    def test_header_mismatch(self):
        a = self.csv_file('a.csv', 'frame,pc\n1,ABCD\n')
        b = self.csv_file('b.csv', 'frame,m\n1,ABCD\n')
        self.assertEqual(run.compare(a, b)['reason'], 'header')

    def test_evidence_is_exclusive(self):
        path = self.root / 'result.json'
        run.save(path, {'passed': False})
        with self.assertRaises(FileExistsError):
            run.save(path, {'passed': True})
        self.assertFalse(json.loads(path.read_text())['passed'])

    def test_cache_preserves_unchanged_source_and_removes_stale_owned_files(self):
        source, dest = self.root / 'source', self.root / 'cache'
        source.mkdir()
        (source / 'a.c').write_text('before')
        run.sync_tree(source, dest)
        before = (dest / 'a.c').stat().st_mtime_ns
        (dest / 'old.c').write_text('stale')
        run.sync_tree(source, dest)
        self.assertEqual((dest / 'a.c').stat().st_mtime_ns, before)
        self.assertFalse((dest / 'old.c').exists())
        (source / 'a.c').write_text('after')
        run.sync_tree(source, dest)
        self.assertEqual((dest / 'a.c').read_text(), 'after')

    def test_changed_input_fails_before_build(self):
        path = self.csv_file('input', 'original')
        config = {'version': 1, 'inputs': {'replay': run.locked(path)}}
        path.write_text('changed')
        with self.assertRaisesRegex(ValueError, 'locked input changed: replay'):
            run.preflight(config)

    def test_anchor_changes_fail_closed(self):
        with self.assertRaises(ValueError):
            instrument.once('missing', 'anchor', 'new')
        with self.assertRaises(ValueError):
            instrument.once('anchor anchor', 'anchor', 'new')

    def test_native_counter_follows_boundary_guard(self):
        source = '#include "funcs.h"\nif (deadline) return yield;\n  _aot_timing = (CpuAotInstructionTiming){2, 12};\n  cpu_aot_insn_bus_extra(&_aot_timing, 7, 0xD8A5, 1);\n'
        text, roots, count = instrument.generated(source, ['07D8A5:1:0'], True)
        self.assertEqual(roots, {0x07D8A5})
        self.assertEqual(count, 1)
        self.assertLess(text.index('if (deadline)'), text.index('st_record_entry(0x'))
        self.assertLess(text.index('st_trace_before(cpu'), text.index('_aot_timing ='))

    def test_private_output_guard(self):
        with self.assertRaises(ValueError):
            run.private(run.ROOT / 'generated')
        with self.assertRaises(ValueError):
            run.private(run.CAPTURES / '..' / 'config')

    def test_failed_command_retains_log_and_status(self):
        log = self.root / 'failed.log'
        with self.assertRaises(RuntimeError):
            run.command([run.sys.executable, '-c', 'print("failure"); raise SystemExit(3)'], log)
        self.assertIn('failure', log.read_text())
        self.assertEqual(json.loads(Path(str(log) + '.json').read_text())['code'], 3)


if __name__ == '__main__':
    unittest.main()
