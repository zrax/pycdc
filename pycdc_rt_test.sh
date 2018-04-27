#!/bin/bash

testdir="$1"
srcdir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
startdir="$(pwd)"

if [[ -z "$testdir" ]]; then
    echo "Missing required parameter: testdir" >&2
    exit 1
fi

py_versions=('1.5.2' '2.2.3' '2.5.6' '2.6.9' '2.7.12' '3.1.5' '3.4.5' '3.5.2')
py_url="http://www.python.org/ftp/python"

# Make subcommand errors fatal
set -e

fetch_python() {
    local url="$1"
    local tarball="${url##*/}"
    local version="$2"

    if [[ ! -d "Python-${version}" ]]; then
        echo "Downloading Python ${version}"
        curl -LfO# "$url"
        tar xzf "$tarball"
        if [[ -f "${srcdir}/scripts/python-builds/Python-${version}.patch" ]]; then
            cd "Python-${version}"
            patch -p1 < "${srcdir}/scripts/python-builds/Python-${version}.patch"
            cd ..
        fi
    fi
}

cd "$startdir"
mkdir -p python-builds
cd python-builds

# Special case for Python 1.5
fetch_python "${py_url}/src/py152.tgz" "1.5.2"

for pv in ${py_versions[@]:1}; do
    fetch_python "${py_url}/${pv}/Python-${pv}.tgz" "$pv"
done

for pv in ${py_versions[@]}; do
    if [[ ! -x "Python-${pv}/python" ]]; then
        echo -n "Building Python ${pv}... "
        cd "Python-${pv}"
        rm -f "../Python-${pv}.conf.log"
        rm -f "../Python-${pv}.build.log"
        if ! ./configure > "../Python-${pv}.conf.log" 2>&1 ; then
            echo "Configure failed!"
            echo "See python-builds/Python-${pv}.conf.log for details"
            exit 1
        fi
        if ! make > "../Python-${pv}.build.log" 2>&1 ; then
            echo "Build failed!"
            echo "See python-builds/Python-${pv}.build.log for details"
            exit 1
        fi
        cd ..
        echo "Success!"
    fi
done

# Run any .pyc.src files in $testdir back through its respective python compiler
cd "$startdir"

fails=0
files=()
errors=()

set +e

shopt -s nullglob
rtsources=("$testdir"/*.pyc.src.py)
shopt -u nullglob

if (( ${#rtsources[@]} == 0 )); then
    echo "No decompyled sources found in $testdir"
    exit 1
fi

for srcf in "${rtsources[@]}"; do
    # There's probably a better way...
    srcver=$(grep '# File: .* (Python [0-9]\.[0-9]\+)$' "$srcf" | sed -e 's/.* (Python //' -e 's/)//')

    _found=0
    for version in ${py_versions[@]}; do
        if [[ "${version%.*}" == "$srcver" ]]; then
            _found=1
            base="tests/$(basename "$srcf")"
            python-builds/Python-${version}/python \
                    -c "import py_compile; py_compile.compile('$srcf')" 2>"$base.rterr"
            if (( $? )) || [[ -s "$base.rterr" ]]; then
                let fails+=1
                files+=("$srcf")
                errors+=("$(cat "$base.rterr")")
                echo -ne "\033[31m.\033[m"
            else
                echo -ne "\033[32m.\033[m"
            fi
        fi
    done

    if (( _found == 0 )); then
        let fails+=1
        files+=("$srcf")
        errors+=("No python compiler found for $srcf ($srcver)")
        echo -ne "\033[31m.\033[m"
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
