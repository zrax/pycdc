#!/bin/bash

srcdir="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
testdir="$srcdir/tests"
testname="$1"
outdir="$2"

if [[ -z "$testname" ]]; then
    echo "Missing required parameter: testname" >&2
    exit 1
fi
if [[ -z "$outdir" ]]; then
    echo "Missing required parameter: outdir" >&2
    exit 1
fi

shopt -s nullglob
compfiles=( "$testdir/compiled/$testname".?.?.pyc )
shopt -u nullglob

if (( ${#compfiles[@]} == 0 )); then
    echo "No compiled modules found for compiled/$testname.*.pyc"
    exit 1
fi

mkdir -p "$outdir"

fails=0
for pyc in "${compfiles[@]}"; do
    base="$outdir/$(basename "$pyc")"

    echo -ne "\033[1m*** $(basename "$pyc"):\033[0m "

    ./pycdc "$pyc" 2>"$base.err" 1>"$base.src.py"
    if (( $? )) || [[ -s "$base.err" ]]
    then
        let fails+=1
        echo -e "\033[31mFAIL\033[m"
        cat "$base.err"
        continue
    fi

    "$srcdir"/scripts/token_dump "$base.src.py" 2>"$base.tok.err" 1>"$base.tok.txt"
    if (( $? )) || [[ -s "$base.tok.err" ]]
    then
        let fails+=1
        echo -e "\033[31mFAIL\033[m"
        cat "$base.tok.err"
        continue
    fi

    if ! diff "$base.tok.txt" "$testdir/tokenized/$testname.txt" >/dev/null
    then
        let fails+=1
        echo -e "\033[31mFAIL\033[m"
        echo "$base.tok.txt does not match $testdir/tokenized/$testname.txt"
        continue
    fi

    echo -e "\033[32mPASS\033[m"
done
