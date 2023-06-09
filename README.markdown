![Tests](https://img.shields.io/github/actions/workflow/status/zrax/pycdc/linux-ci.yml?branch=master&label=tests&style=flat-square)
[![PyPI](https://img.shields.io/pypi/v/pycdc?color=blue&style=flat-square)](https://pypi.org/project/pycdc/)
[![Python](https://img.shields.io/pypi/pyversions/pycdc?color=blue&style=flat-square)](https://pypi.org/project/pycdc/)

# Decompyle++ 
***A Python Byte-code Disassembler/Decompiler***

Decompyle++ aims to translate compiled Python byte-code back into valid
and human-readable Python source code. While other projects have achieved
this with varied success, Decompyle++ is unique in that it seeks to
support byte-code from any version of Python.

Decompyle++ includes both a byte-code disassembler (pycdas) and a 
decompiler (pycdc).

As the name implies, Decompyle++ is written in C++.
If you wish to contribute, please fork us on github at 
https://github.com/zrax/pycdc

## Building Decompyle++
* Generate a project or makefile with [CMake](http://www.cmake.org) (See CMake's documentation for details)
  * The following options can be passed to CMake to control debug features:

    | Option | Description |
    | --- | --- |
    | `-DCMAKE_BUILD_TYPE=Debug` | Produce debugging symbols |
    | `-DENABLE_BLOCK_DEBUG=ON` | Enable block debugging output |
    | `-DENABLE_STACK_DEBUG=ON` | Enable stack debugging output |

* Build the generated project or makefile
  * For projects (e.g. MSVC), open the generated project file and build it
  * For makefiles, just run `make`
  * To run tests (on \*nix or MSYS), run `make check`

## Building and installing the Python package

This step does not require building the executables of the previous sections.

* Ensure `CMake >= 3.12` is installed
* Create a virtual environment `python3 -m venv venv`
* Run `pip install .`

## Usage
**To run pycdas**, the PYC Disassembler:
`./pycdas [PATH TO PYC FILE]`
The byte-code disassembly is printed to stdout.

**To run pycdc**, the PYC Decompiler: 
`./pycdc [PATH TO PYC FILE]`
The decompiled Python source is printed to stdout.
Any errors are printed to stderr.

**Marshalled code objects**:
Both tools support Python marshalled code objects, as output from `marshal.dumps(compile(...))`.

To use this feature, specify `-c -v <version>` on the command line - the version must be specified as the objects themselves do not contain version metadata.

**To use the Python bindings**

Ensure the `pycdc` Python package is installed:

- either from source (see [above](#building-and-installing-the-python-package))
- or from PyPi : `pip install pycdc`

Then, use it as below:

```python
import marshal
from pycdc import decompyle

async def test():
    a = 5
    data = foobar(a)
    return data

print(decompyle(marshal.dumps(test.__code__)))
```

or from a `.pyc` file:

```python
from pycdc import decompyle

with open('test.pyc', 'rb') as f:
    # pass version=None to infer from the file, or specify a version tuple
    print(decompyle(f.read(), version=None))
```

## Authors, Licence, Credits
Decompyle++ is the work of Michael Hansen and Darryl Pogue.

Additional contributions from:
* charlietang98
* Kunal Parmar
* Olivier Iffrig
* Zlodiy

It is released under the terms of the GNU General Public License, version 3;
See LICENSE file for details.
