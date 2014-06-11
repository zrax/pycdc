#!/bin/bash

mkdir tests

fails=0
files=()
errors=()
for f in $1/*.pyc
do
    base=tests/$( basename "$f" )
    stderr=$( ./pycdc "$f" 2>$base.err 1>$base.src )
    if [ "$?" -eq "0" -a -z "$stderr" ]
    then
        echo -ne "\033[32m.\033[m"
    else
        let fails+=1
        files=("${files[@]}" "$f")
        errors=("${errors[@]}" "$stderr")
        echo -ne "\033[31m.\033[m"
    fi
done
echo -e "\n\n$fails tests failed:"
for ((i=0; i<${#files[@]}; i++))
do
    echo -e "\t\033[31m${files[i]}\033[m"
echo -e "${errors[i]}\n"
done
