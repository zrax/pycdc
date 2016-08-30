#!/bin/bash

testdir="$1"
mkdir -p tests

fails=0
files=()
errors=()
for f in "$testdir"/*.pyc
do
    base="tests/$(basename "$f")"
    ./pycdc "$f" 2>"$base.err" 1>"$base.src"
    if (( $? )) || [[ -s "$base.err" ]]
    then
        let fails+=1
        files+=("$f")
        errors+=("$(cat "$base.err")")
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
