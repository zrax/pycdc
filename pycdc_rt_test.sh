#!/bin/bash

testdir="$1"
srcdir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
startdir="$(pwd)"

if [[ -z "$testdir" ]]; then
    echo "Missing required parameter: testdir" >&2
    exit 1
fi

if [[ ! -d "$testdir" ]]; then
    echo "Test directory $testdir does not exist" >&2
    exit 1
fi
cd "$testdir"

# Run any .pyc.src files in $testdir back through its respective python compiler
fails=0
files=()
errors=()

shopt -s nullglob
rtsources=(*.pyc.src.py)
shopt -u nullglob

if (( ${#rtsources[@]} == 0 )); then
    echo "No decompyled sources found in $testdir"
    exit 1
fi

for srcf in "${rtsources[@]}"; do
    # There's probably a better way...
    srcver=$(grep '# File: .* (Python [0-9]\.[0-9]\+)$' "$srcf" | sed -e 's/.* (Python //' -e 's/)//')

    base="$(basename "$srcf")"
    "$srcdir"/scripts/pymultic "$srcver" "$srcf" 2> "$base.rterr"
    if (( $? )) || [[ -s "$base.rterr" ]]; then
        let fails+=1
        files+=("$srcf")
        errors+=("$(cat "$base.rterr")")
    fi
done

if (( $fails == 0 ))
then
    echo -e "\n\nAll tests passed!"
else
    echo -e "\n\n$fails tests failed:"
    for ((i=0; i<${#files[@]}; i++))
    do
        echo -e "\t\033[31m${files[i]}\033[m"
        echo -e "${errors[i]}\n"
    done
fi
