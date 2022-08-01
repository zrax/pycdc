#!/bin/bash

srcdir="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
testdir="$srcdir/tests"
testname="$1"
outdir="$2"

if [[ -z "$PYTHON_EXE" ]]; then
    PYTHON_EXE="$(which python3)"
fi

if [[ -z "$testname" ]]; then
    echo "Missing required parameter: testname" >&2
    exit 1
fi
if [[ -z "$outdir" ]]; then
    echo "Missing required parameter: outdir" >&2
    exit 1
fi

shopt -s nullglob
compfiles=( "$testdir/compiled/$testname".?.?*.pyc )
xfcfiles=( "$testdir/xfail/$testname".?.?*.pyc )
shopt -u nullglob

if (( ${#compfiles[@]} + ${#xfcfiles[@]} == 0 )); then
    echo "No compiled/xfail modules found for $testname.*.pyc"
    exit 1
fi

mkdir -p "$outdir"

echo -ne "\033[1m*** $testname:\033[0m "

fails=0
xfails=0
upass=0
efiles=()
errors=()
upfiles=()
for pyc in "${compfiles[@]}" "${xfcfiles[@]}"; do
    base="$outdir/$(basename "$pyc")"

    ./pycdc "$pyc" -o "$base.src.py" >"$base.err" 2>&1
    if (( $? )) || [[ -s "$base.err" ]]
    then
        if [[ "$(dirname "$pyc")" =~ xfail ]]
        then
            let xfails+=1
        else
            let fails+=1
            efiles+=("$(basename "$pyc")")
            errors+=("$(cat "$base.err")")
        fi
        continue
    fi

    "$PYTHON_EXE" "$srcdir"/scripts/token_dump "$base.src.py" 2>"$base.tok.err" 1>"$base.tok.txt"
    if (( $? )) || [[ -s "$base.tok.err" ]]
    then
        if [[ "$(dirname "$pyc")" =~ xfail ]]
        then
            let xfails+=1
        else
            let fails+=1
            efiles+=("$(basename "$pyc")")
            errors+=("$(cat "$base.tok.err")")
        fi
        continue
    fi

    diff -u "$testdir/tokenized/$testname.txt" "$base.tok.txt" >"$base.tok.diff"
    if (( $? ))
    then
        if [[ "$(dirname "$pyc")" =~ xfail ]]
        then
            let xfails+=1
        else
            let fails+=1
            efiles+=("$(basename "$pyc")")
            errors+=("$base.tok.txt does not match $testdir/tokenized/$testname.txt:\n$(cat "$base.tok.diff")")
        fi
    else
        if [[ "$(dirname "$pyc")" =~ xfail ]]
        then
            let upass+=1
            upfiles+=("$(basename "$pyc")")
        fi
    fi
done

if (( $fails == 0 ))
then
    if (( $xfails != 0 ))
    then
        if (( ${#compfiles[@]} == 0 ))
        then
            echo -e "\033[33mXFAIL ($xfails)\033[0m"
        else
            echo -e "\033[32mPASS (${#compfiles[@]})\033[33m + XFAIL ($xfails)\033[0m"
        fi
    else
        echo -e "\033[32mPASS (${#compfiles[@]})\033[0m"
    fi
else
    if (( $xfails != 0 ))
    then
        echo -e "\033[31mFAIL ($fails of ${#compfiles[@]})\033[33m + XFAIL ($xfails)\033[0m"
    else
        echo -e "\033[31mFAIL ($fails of ${#compfiles[@]})\033[0m"
    fi

    for ((i=0; i<${#efiles[@]}; i++))
    do
        echo -e "\t\033[31m${efiles[i]}\033[0m"
        echo -e "${errors[i]}\n"
    done
fi

if (( $upass != 0 ))
then
    echo -e "\033[1;34mUnexpected passes:\033[0m"
    for ((i=0; i<${#upfiles[@]}; i++))
    do
        echo -e "\t\033[33m${upfiles[i]}\033[0m"
    done
fi

if (( $fails != 0 ))
then
    exit 1
fi
