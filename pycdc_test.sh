#!/bin/bash

# TODO: Consider making a non-empty stderr be a failure
# result, however this can make understanding new versions
# harder to upgrade with.
#
# Possibly need a 'warning' level from test output.
mkdir -p tests

fails=0
files=()
errors=()
diffs=()
for prog in pycdc pycdas
do
    for f in $(find $1/inputs -name '*.pyc' -or -name '*.pyo')
    do
        rel=$(realpath --relative-to=$1/inputs $f)
        baseline=$1/baseline/$prog/$rel
        out=tests/$prog/$rel
        mkdir -p $(dirname $out)
        if ! ./$prog "$f" 2>$out.err 1>$out.src
        then
            let fails+=1
            files=("${files[@]}" "$rel")
            stderr=$(cat $out.err)
            errors=("${errors[@]}" "Bad exit code\n$stderr")
            echo -ne "\033[31m.\033[m"
        elif ! diff $out.err $baseline.err >/dev/null 2>&1
        then
            let fails+=1
            files=("${files[@]}" "$rel")
            errdiff=$(diff $out.err $baseline.err)
            errors=("${errors[@]}" "Mismatch stderr\n$errdiff")
            echo -ne "\033[31m.\033[m"
        elif ! diff $out.src $baseline.src >/dev/null 2>&1
        then
            let fails+=1
            files=("${files[@]}" "$rel")
            srcdiff=$(diff $out.src $baseline.src)
            errors=("${errors[@]}" "Mismatch stdout\n$srcdiff")
            echo -ne "\033[31m.\033[m"
        else
            echo -ne "\033[32m.\033[m"
        fi
    done
done
echo -e "\n\n$fails tests failed:"
for ((i=0; i<${#files[@]}; i++))
do
    echo -e "\t\033[31m${files[i]}\033[m"
echo -e "${errors[i]}\n"
done
