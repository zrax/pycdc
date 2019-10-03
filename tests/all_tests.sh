#!/bin/bash

srcdir="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"

test_status=0
test_files=( "$srcdir"/tests/tokenized/*.txt )
for tf in "${test_files[@]}"; do
    test_name="$(basename "$tf")"
    test_name="${test_name%.txt}"
    "$srcdir"/tests/decompyle_test.sh $test_name tests || test_status=1
done

exit $test_status
