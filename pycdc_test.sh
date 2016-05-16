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
for f in $(find $1/inputs -name '*.pyc' -or -name '*.pyo')
do
    rel=$(realpath --relative-to=$1/inputs $f)
    base=tests/$rel
    mkdir -p $(dirname $base)
    if ! ./pycdc "$f" 2>$base.err 1>$base.src
    then
        let fails+=1
        files=("${files[@]}" "$rel")
        stderr=$(cat $base.err)
        errors=("${errors[@]}" "Bad exit code\n$stderr")
        echo -ne "\033[31m.\033[m"
    elif ! diff $base.err $1/baseline/$rel.err >/dev/null 2>&1
    then
        let fails+=1
        files=("${files[@]}" "$rel")
        errdiff=$(diff $base.err $1/baseline/$rel.err)
        errors=("${errors[@]}" "Mismatch stderr\n$errdiff")
        echo -ne "\033[31m.\033[m"
    elif ! diff $base.src $1/baseline/$rel.src >/dev/null 2>&1
    then
        let fails+=1
        files=("${files[@]}" "$rel")
        srcdiff=$(diff $base.src $1/baseline/$rel.src)
        errors=("${errors[@]}" "Mismatch stdout\n$srcdiff")
        echo -ne "\033[31m.\033[m"
    else
        echo -ne "\033[32m.\033[m"
    fi
done
echo -e "\n\n$fails tests failed:"
for ((i=0; i<${#files[@]}; i++))
do
    echo -e "\t\033[31m${files[i]}\033[m"
echo -e "${errors[i]}\n"
done
