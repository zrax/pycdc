#!/usr/bin/env python3

import os
import sys
import glob
import difflib
import argparse
import subprocess
import multiprocessing

TEST_DIR = os.path.dirname(os.path.realpath(__file__))
SCRIPTS_DIR = os.path.realpath(os.path.join(TEST_DIR, '..', 'scripts'))

def decompyle_one(test_name, pyc_file, outdir, tokenized_expect):
    out_base = os.path.join(outdir, os.path.basename(pyc_file))
    proc = subprocess.run(
            [os.path.join(os.getcwd(), 'pycdc'), pyc_file, '-o', out_base + '.src.py'],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True,
            encoding='utf-8', errors='replace')
    pycdc_output = proc.stdout
    if proc.returncode != 0 or pycdc_output:
        with open(out_base + '.err', 'w') as errfile:
            errfile.write(pycdc_output)
        return False, [pycdc_output]
    elif os.path.exists(out_base + '.err'):
        os.unlink(out_base + '.err')

    proc = subprocess.run(
            [sys.executable, os.path.join(SCRIPTS_DIR, 'token_dump'), out_base + '.src.py'],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True,
            encoding='utf-8', errors='replace')
    tokenized = proc.stdout
    token_dump_err = proc.stderr
    with open(out_base + '.tok.txt', 'w') as tokfile:
        tokfile.write(tokenized)
    if proc.returncode != 0 or token_dump_err:
        with open(out_base + '.tok.err', 'w') as errfile:
            errfile.write(token_dump_err)
        return False, [token_dump_err]
    elif os.path.exists(out_base + '.tok.err'):
        os.unlink(out_base + '.tok.err')

    if tokenized != tokenized_expect:
        fromfile = 'tokenized/{}.txt'.format(test_name)
        tofile = 'tests-out/{}.tok.txt'.format(os.path.basename(pyc_file))
        diff = difflib.unified_diff(tokenized_expect.splitlines(True), tokenized.splitlines(True),
                                    fromfile=fromfile, tofile=tofile)
        diff = list(diff)
        with open(out_base + '.tok.diff', 'w') as diff_file:
            diff_file.writelines(diff)
        return False, ['Tokenized output does not match expected output:\n'] + diff

    return True, []


def run_test(test_file):
    """
    Runs a single test, and returns a tuple containing the number of failed
    tests and the output of the test.  The output is not printed directly
    in order to avoid interleaving output from multiple parallel tests.
    """
    test_name = os.path.splitext(os.path.basename(test_file))[0]
    compiled_files = glob.glob(os.path.join(TEST_DIR, 'compiled', test_name + '.?.*.pyc'))
    xfail_files = glob.glob(os.path.join(TEST_DIR, 'xfail', test_name + '.?.*.pyc'))
    if not compiled_files and not xfail_files:
        return 1, 'No compiled/xfail modules found for {}\n'.format(test_name)

    outdir = os.path.join(os.getcwd(), 'tests-out')
    os.makedirs(outdir, exist_ok=True)

    with open(os.path.join(TEST_DIR, 'tokenized', test_name + '.txt'), 'r',
              encoding='utf-8', errors='replace') as tok_file:
        tokenized_expect = tok_file.read()

    status_line = '\033[1m*** {}:\033[0m '.format(test_name)
    errlines = []
    fails = 0
    xfails = 0
    upass = 0
    for xpass_file in compiled_files:
        ok, errs = decompyle_one(test_name, xpass_file, outdir, tokenized_expect)
        if not ok:
            fails += 1
            errlines.append('\t\033[31m{}\033[0m\n'.format(os.path.basename(xpass_file)))
            errlines.extend(errs)
    for xfail_file in xfail_files:
        ok, _ = decompyle_one(test_name, xfail_file, outdir, tokenized_expect)
        if not ok:
            xfails += 1
        else:
            upass += 1

    if fails == 0:
        if xfails != 0:
            if not compiled_files:
                status_line += '\033[33mXFAIL ({})\033[0m\n'.format(xfails)
            else:
                status_line += '\033[32mPASS ({})\033[33m + XFAIL ()\033[0m\n' \
                               .format(len(compiled_files), xfails)
        else:
            status_line += '\033[32mPASS ({})\033[0m\n'.format(len(compiled_files))
    else:
        if xfails != 0:
            status_line += '\033[31mFAIL ({} of {})\033[33m + XFAIL ({})\033[0m\n' \
                           .format(fails, len(compiled_files), xfails)
        else:
            status_line += '\033[31mFAIL ({} of {})\033[0m\n'.format(fails, len(compiled_files))

    return fails, [status_line] + errlines


def main():
    # For simpler invocation from CMake's check target, we also support setting
    # these parameters via environment variables.
    default_jobs = int(os.environ['JOBS']) if 'JOBS' in os.environ else multiprocessing.cpu_count()
    default_filter = os.environ['FILTER'] if 'FILTER' in os.environ else ''

    parser = argparse.ArgumentParser()
    parser.add_argument('--jobs', '-j', type=int, default=default_jobs,
            help='Number of tests to run in parallel (default: {})'.format(default_jobs))
    parser.add_argument('--filter', type=str, default=default_filter,
            help='Run only test(s) matching the supplied filter')
    args = parser.parse_args()

    glob_pattern = '*{}*.txt'.format(args.filter) if args.filter else '*.txt'
    test_files = sorted(glob.iglob(os.path.join(TEST_DIR, 'tokenized', glob_pattern)))
    total_fails = 0
    with multiprocessing.Pool(args.jobs) as pool:
        for fails, output in pool.imap(run_test, test_files):
            total_fails += fails
            sys.stdout.writelines(output)

    if total_fails:
        print('{} test(s) failed'.format(total_fails))
        sys.exit(1)

if __name__ == '__main__':
    main()
