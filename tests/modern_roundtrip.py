#!/usr/bin/env python3
"""Compile authored fixtures and compare the recovered programs' observable values.

Run with the bytecode-producing interpreter (CPython 3.13 or 3.14):
    python tests/modern_roundtrip.py --exe /path/to/pycdc
Only these controlled fixtures are executed. Output is retained in --workdir.
This supplements run_tests.py's historical token comparison, without requiring
checked-in, platform-specific pyc files or claiming general version support.
"""
import argparse
import ast
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import threading
import time


CASES = {
    "functions": """
def combine(a=3, /, *args, first=5, required, last=7, **kwargs):
    return (a, args, first, required, last, kwargs)
f = lambda a=2, /, *, required=4: a * 10 + required
result = (combine(1, 2, 3, last=6, required=8, x=9), f(required=5),
          combine.__defaults__, combine.__kwdefaults__)
""",
    "keyword_order": """
events = []
def mark(value):
    events.append(value)
    return value
def combine(a, *, first, last):
    return a * 100 + first * 10 + last
result = (combine(mark(1), last=mark(2), first=mark(3)), events,
          'abcabc'.replace('a', 'x', 1))
""",
    "closure": """
def outer(x):
    def inner(a=2, *, scale=3):
        return (x + a) * scale
    return inner
f = outer(10)
result = (f(), f(5, scale=4))
""",
    "decorator": """
def decorate(f):
    f.tag = f.__name__
    return f
@decorate
def named(a=2):
    x = a + 3
    return x * 2
result = (named(), named.tag)
""",
    "truth": """
class Flag:
    def __init__(self, value):
        self.value = value
        self.count = 0
    def __bool__(self):
        self.count += 1
        return self.value
def convert(value):
    return not not value
bool = 99
empty = Flag(False)
full = Flag(True)
result = (convert(empty), convert(full), empty.count, full.count)
""",
    "short_circuit": """
def both(a, b):
    return a and b
def either(a, b):
    return a or b
def mixed(a, b, c):
    return (a and b) or c
result = (both([], [1]), both([1], [2]), either([], [3]), either([4], [5]),
          mixed([], [1], [2]), mixed([1], [], [3]), mixed([1], [2], [3]))
""",
    "swap_and_subscript": """
def calc(a, b):
    x = a + 1
    y = b + 2
    return x * 10 + y
def swap(a, b):
    a, b = b, a
    return a * 10 + b
def get(values):
    return values[0] + values[1]
result = (calc(3, 5), swap(1, 2), get([10, 20]))
""",
    "star_call": """
def combine(a, b=3):
    return a * 10 + b
args = (2, 4)
kwargs = {'b': 6}
result = (combine(*args), combine(*(5,), **kwargs))
""",
    "loop_and_return": """
def choose(value):
    if value:
        return 11
    return 22
def parse(items):
    result = []
    for item in items:
        if item:
            result.append(item + 1)
        else:
            result.append(9)
    return result
result = (choose([]), choose([1]), parse([0, 1, 2]))
""",
}


def run(command):
    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            start_new_session=os.name != "nt")

    def kill_tree():
        if os.name == "nt":
            subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        if proc.poll() is None:
            proc.kill()

    output = [bytearray(), bytearray()]
    exceeded = threading.Event()

    def read(pipe, dest):
        while chunk := pipe.read(65536):
            if len(dest) + len(chunk) > 2 * 1024 * 1024:
                exceeded.set()
                kill_tree()
                break
            dest.extend(chunk)
        pipe.close()

    threads = [threading.Thread(target=read, args=(pipe, dest), daemon=True)
               for pipe, dest in zip((proc.stdout, proc.stderr), output)]
    for thread in threads:
        thread.start()
    started = time.monotonic()
    while proc.poll() is None or any(thread.is_alive() for thread in threads):
        if exceeded.is_set() or time.monotonic() - started > 10:
            kill_tree()
            for thread in threads:
                thread.join(3)
            proc.wait(timeout=3)
            raise RuntimeError("output budget exceeded" if exceeded.is_set() else "timeout; process tree killed")
        time.sleep(0.01)
    out, err = map(bytes, output)
    if proc.returncode or err:
        raise RuntimeError("rc=%d: %s" % (proc.returncode, err.decode(errors="replace")[:300]))
    return out


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--workdir", required=True, type=Path,
                        help="existing parent for retained test evidence")
    args = parser.parse_args()
    if sys.version_info[:2] not in ((3, 13), (3, 14)):
        parser.error("use CPython 3.13 or 3.14 to generate real bytecode")
    if not args.workdir.is_dir() or not args.exe.is_file():
        parser.error("binary and evidence parent must exist")
    work = Path(tempfile.mkdtemp(prefix="pycdc-roundtrip-", dir=args.workdir))
    probe = "import sys; ns={}; exec(compile(open(sys.argv[1]).read(), sys.argv[1], 'exec'), ns); print(repr(ns['result']))"
    failed = 0
    cases = dict(CASES)
    if sys.version_info[:2] == (3, 13):
        # 3.14 deferred annotations (function attribute 16) are deliberately
        # rejected by the candidate and are outside this passing matrix.
        cases["annotations"] = "def f(a: int = 3) -> int:\n    return a + 1\nresult = (f(), f.__annotations__)\n"
    for name, source in cases.items():
        try:
            src = work / (name + ".py")
            pyc = src.with_suffix(".pyc")
            recovered = work / (name + "-recovered.py")
            src.write_text(source, encoding="utf-8")
            run([sys.executable, "-c", "import py_compile,sys; py_compile.compile(sys.argv[1],cfile=sys.argv[2],doraise=True)",
                 str(src), str(pyc)])
            out = run([str(args.exe.resolve()), str(pyc)])
            text = out.decode("utf-8")
            assert ast.parse(text).body and "Decompyle incomplete" not in text
            recovered.write_text(text, encoding="utf-8")
            assert run([sys.executable, "-c", probe, str(src)]) == run([sys.executable, "-c", probe, str(recovered)])
            print("PASS", name)
        except (OSError, RuntimeError, SyntaxError, AssertionError) as exc:
            failed += 1
            print("FAIL", name, exc)
    print("EVIDENCE", work)
    print("TOTAL", len(cases) - failed, "PASS", failed, "FAIL")
    return bool(failed)


if __name__ == "__main__":
    raise SystemExit(main())
