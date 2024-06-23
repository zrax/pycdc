#!/bin/bash
set -e

srcdir="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
jobs=${JOBS:-4}
filter=${FILTER:-""}

find "${srcdir}/tests/tokenized" -type f -name '*.txt' -a -name "*${filter}*" -print0 | \
    xargs -0 -I '{}' -P $jobs \
          bash -c 'o=$('"$srcdir"'/tests/decompyle_test.sh "$(basename -s .txt "{}")" tests-out) r=$?; echo "$o"; exit $r'
