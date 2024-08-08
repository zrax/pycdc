#!/usr/bin/env python3

import os
import sys
import glob
import argparse
import subprocess
import multiprocessing

TEST_DIR = os.path.dirname(os.path.realpath(__file__))

def run_test(test_file):
    test_name = os.path.splitext(os.path.basename(test_file))[0]
    proc = subprocess.run(
            [os.path.join(TEST_DIR, 'decompyle_test.sh'), test_name, 'tests-out'],
            check=True, stdout=subprocess.PIPE)
    return proc.stdout.decode('utf-8')

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
    with multiprocessing.Pool(args.jobs) as pool:
        for line in pool.imap(run_test, test_files):
            sys.stdout.write(line)

if __name__ == '__main__':
    main()
