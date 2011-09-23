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

## Usage
To compile Decompyle++, just run `make`.

**To run pycdas**, the PYC Disassembler:
`./bin/pycdas [PATH TO PYC FILE]`
The byte-code disassembly is printed to stdout.

**To run pycdc**, the PYC Decompiler: 
`./bin/pycdc [PATH TO PYC FILE]`
The decompiled Python source is printed to stdout.
Any errors are printed to stderr.

## Authors, Licence, Credits
Decompyle++ is the work of Michael Hansen and Darryl Pogue.
It is released under the terms of the GNU General Public License, version 3.
