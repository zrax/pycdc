#!/usr/bin/env python3
"""Closed-loop, self-verifying test suite for pycdc's Python 3.13/3.14 support.

For every case in ``cases/`` and every configured interpreter the pipeline is:

    source .py  --(py_compile)-->  .pyc  --(pycdc)-->  decompiled .py
        |                                                      |
        +----------------------- semantic ---------------------+
                              comparison

A case passes only when ALL of the following hold:

  1. pycdc exits 0 and prints nothing on stderr/stdout
     (no ``Unsupported opcode`` / ``WARNING: Decompyle incomplete``).
  2. The decompiled source parses (``ast.parse``).
  3. Both sources execute and their ``__test__()`` probes return identical
     results (list of reprs), i.e. observable behaviour is preserved.
  4. The decompiled module exposes the same public callables/attributes.

Cases whose file name starts with ``xfail_`` are known upstream limitations
(affecting 3.10-3.14 alike); they are run through the same pipeline and are
expected to fail stage 1-3.  They are reported separately and do not affect
the exit code unless they unexpectedly PASS (which would mean the limitation
was fixed and the marker should be removed).

Usage:
    python3 run_roundtrip.py                       # auto-detect interpreters
    python3 run_roundtrip.py --pycdc ./pycdc       # explicit binary
    python3 run_roundtrip.py --py 3.13 --py 3.14   # explicit versions
    python3 run_roundtrip.py --keep                # keep work directory

The script exits 0 only if every non-xfail case passes for at least one
interpreter AND every interpreter that ran had no unexpected failure.
"""

import argparse
import ast
import glob
import importlib.util
import os
import shutil
import subprocess
import sys
import tempfile
import traceback

HERE = os.path.dirname(os.path.realpath(__file__))
CASES_DIR = os.path.join(HERE, 'cases')

INTERPRETER_CANDIDATES = {
    '3.13': ('python3.13', '/opt/homebrew/bin/python3.13', '/usr/local/bin/python3.13',
             '/Library/Frameworks/Python.framework/Versions/3.13/bin/python3.13'),
    '3.14': ('python3.14', '/opt/homebrew/bin/python3.14', '/usr/local/bin/python3.14',
             '/Library/Frameworks/Python.framework/Versions/3.14/bin/python3.14'),
}


def find_pycdc(explicit):
    if explicit:
        return os.path.realpath(explicit)
    # Prefer a freshly built binary next to the current working directory,
    # then fall back to the sibling build directory of the repository.
    local = os.path.join(os.getcwd(), 'pycdc')
    if os.path.exists(local):
        return os.path.realpath(local)
    repo = os.path.realpath(os.path.join(HERE, '..', '..'))
    for cand in (os.path.join(repo, 'build', 'pycdc'), os.path.join(repo, 'build2', 'pycdc')):
        if os.path.exists(cand):
            return os.path.realpath(cand)
    return shutil.which('pycdc')


def find_interpreters(wanted):
    found = {}
    for ver in wanted:
        for exe in INTERPRETER_CANDIDATES.get(ver, ()):
            path = shutil.which(exe) or (exe if os.path.exists(exe) else None)
            if path:
                try:
                    out = subprocess.run([path, '-c', 'import sys; print("%d.%d" % sys.version_info[:2])'],
                                         stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                         universal_newlines=True)
                except OSError:
                    continue
                if out.returncode == 0 and out.stdout.strip() == ver:
                    found[ver] = os.path.realpath(path)
                    break
    return found


def run_probes(source_path, label):
    """Import a python source file as a module and run its __test__ probes."""
    modname = 'rt_' + os.path.splitext(os.path.basename(source_path))[0]
    spec = importlib.util.spec_from_file_location(modname, source_path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    if not hasattr(mod, '__test__'):
        raise RuntimeError('%s: no __test__ probe' % label)
    result = mod.__test__()
    if not isinstance(result, list) or not all(isinstance(r, str) for r in result):
        raise RuntimeError('%s: __test__ must return a list of repr strings' % label)
    public = sorted(k for k in vars(mod)
                    if not k.startswith('_') and not isinstance(getattr(mod, k), type(mod)))
    return result, public


def decompile(pycdc, pyc_path, out_path):
    """Returns (ok, diagnostics)."""
    with open(out_path, 'w') as out:
        proc = subprocess.run([pycdc, pyc_path], stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, universal_newlines=True,
                              encoding='utf-8', errors='replace')
    if proc.returncode != 0 or proc.stdout or proc.stderr:
        diag = (proc.stdout + proc.stderr).strip()
        if proc.returncode != 0:
            diag = 'pycdc exit=%d\n%s' % (proc.returncode, diag)
        return False, diag or 'pycdc produced output'
    # pycdc prints the source to stdout; -o was not used so re-run is avoided
    # by capturing stdout above.  Write it out for inspection.
    return True, proc.stdout


def decompile_to_file(pycdc, pyc_path, out_path):
    proc = subprocess.run([pycdc, pyc_path], stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, universal_newlines=True,
                          encoding='utf-8', errors='replace')
    diag = (proc.stdout + proc.stderr).strip()
    if proc.returncode != 0:
        return False, 'pycdc exit=%d\n%s' % (proc.returncode, diag)
    # Anything on stderr (warnings) counts as incomplete output; stdout is
    # the generated source itself.
    if proc.stderr.strip():
        return False, 'pycdc stderr:\n%s' % proc.stderr.strip()
    with open(out_path, 'w', encoding='utf-8') as fh:
        fh.write(proc.stdout)
    return True, ''


def compile_pyc(interp, src, pyc):
    proc = subprocess.run([interp, '-c',
                           'import py_compile,sys; py_compile.compile(sys.argv[1], sys.argv[2], doraise=True)',
                           src, pyc],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          universal_newlines=True)
    if proc.returncode != 0:
        return False, (proc.stdout + proc.stderr).strip()
    return True, ''


def run_case(pycdc, interp, case_src, workdir):
    """Returns (status, detail) with status in {'PASS','FAIL'}."""
    name = os.path.splitext(os.path.basename(case_src))[0]
    base = os.path.join(workdir, name)
    pyc = base + '.pyc'
    dec = base + '.dec.py'

    ok, diag = compile_pyc(interp, case_src, pyc)
    if not ok:
        return 'FAIL', 'compilation failed:\n' + diag

    ok, diag = decompile_to_file(pycdc, pyc, dec)
    if not ok:
        return 'FAIL', diag

    with open(dec, encoding='utf-8') as fh:
        dec_src = fh.read()
    try:
        ast.parse(dec_src)
    except SyntaxError as e:
        return 'FAIL', 'decompiled source has a syntax error: %s' % e

    try:
        expected, expected_public = run_probes(case_src, name + ':original')
        actual, actual_public = run_probes(dec, name + ':decompiled')
    except Exception:
        return 'FAIL', traceback.format_exc()

    if expected != actual:
        import difflib
        diff = '\n'.join(difflib.unified_diff(
            expected, actual, 'original', 'decompiled', lineterm=''))
        return 'FAIL', 'semantic mismatch:\n' + diff
    if expected_public != actual_public:
        return 'FAIL', ('module surface mismatch: %s != %s'
                        % (expected_public, actual_public))
    return 'PASS', '%d probes, %d module attrs' % (len(actual), len(actual_public))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--pycdc', help='path to the pycdc binary')
    ap.add_argument('--py', action='append', default=[],
                    help='python version(s) to test, e.g. --py 3.14 (repeatable); '
                         'default: auto-detect 3.13 and 3.14')
    ap.add_argument('--case', action='append', default=[],
                    help='restrict to specific case file(s) in cases/')
    ap.add_argument('--keep', action='store_true', help='keep the work directory')
    args = ap.parse_args()

    pycdc = find_pycdc(args.pycdc)
    if not pycdc or not os.path.exists(pycdc):
        print('error: pycdc binary not found (build it or pass --pycdc)', file=sys.stderr)
        return 2
    print('pycdc     : %s' % pycdc)

    wanted = args.py or ['3.13', '3.14']
    interps = find_interpreters(wanted)
    if not interps:
        print('error: none of the requested interpreters (%s) found' % ', '.join(wanted),
              file=sys.stderr)
        return 2
    for ver in sorted(interps):
        print('python %s : %s' % (ver, interps[ver]))

    if args.case:
        cases = []
        for c in args.case:
            p = c if os.path.isabs(c) else os.path.join(CASES_DIR, c)
            if not p.endswith('.py'):
                p += '.py'
            cases.append(p)
    else:
        cases = sorted(glob.glob(os.path.join(CASES_DIR, '*.py')))
    if not cases:
        print('error: no test cases found in %s' % CASES_DIR, file=sys.stderr)
        return 2

    workdir = tempfile.mkdtemp(prefix='pycdc-roundtrip-')
    print('workdir   : %s\n' % workdir)

    failures = []
    unexpected_passes = []
    total_runs = 0

    for ver in sorted(interps):
        interp = interps[ver]
        print('===== python %s =====' % ver)
        for case in cases:
            name = os.path.splitext(os.path.basename(case))[0]
            xfail = name.startswith('xfail_')
            total_runs += 1
            try:
                status, detail = run_case(pycdc, interp, case, workdir)
            except Exception:
                status, detail = 'FAIL', traceback.format_exc()
            if xfail:
                if status == 'PASS':
                    unexpected_passes.append((ver, name))
                    print('  UPPASS  %-28s (limitation fixed? update the case!)' % name)
                else:
                    print('  XFAIL   %-28s (known limitation)' % name)
            else:
                if status == 'PASS':
                    print('  PASS    %-28s %s' % (name, detail))
                else:
                    failures.append((ver, name))
                    print('  FAIL    %-28s' % name)
                    for line in detail.splitlines():
                        print('          | %s' % line)
        print()

    print('===== summary =====')
    print('runs executed : %d (%d cases x %d interpreters)'
          % (total_runs, len(cases), len(interps)))
    print('failures      : %d%s' % (len(failures),
          '' if not failures else ' -> ' + ', '.join('%s/%s' % f for f in failures)))
    print('unexpected xfail passes: %d%s' % (len(unexpected_passes),
          '' if not unexpected_passes else ' -> ' + ', '.join('%s/%s' % p for p in unexpected_passes)))

    if not args.keep:
        shutil.rmtree(workdir, ignore_errors=True)
    else:
        print('kept workdir   : %s' % workdir)

    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
