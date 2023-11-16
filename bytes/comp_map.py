#!/usr/bin/env python

# This file is part of pycdc.
#
# pycdc is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# pycdc is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with pycdc.  If not, see <http://www.gnu.org/licenses/>.

import sys
import os

if len(sys.argv) != 3:
    sys.stderr.write('Usage: %s in_dir out_dir\n' % sys.argv[0])
    sys.exit(1)

if not os.path.exists(sys.argv[2]):
    os.mkdir(sys.argv[2])

maplist = [ 10, 11,     13, 14, 15, 16,
            20, 21, 22, 23, 24, 25, 26, 27,
            30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 310, 311, 312 ]

for mapver in maplist:
    infile = open(os.path.join(sys.argv[1], 'python_%d.map' % mapver), 'rt')
    outfile = open(os.path.join(sys.argv[2], 'python_%d.cpp' % mapver), 'wt')

    idToOpcode = {}
    for ln in infile.readlines():
        fileid, code = ln.split()
        idToOpcode[int(fileid)] = code

    outfile.write('/* This file was auto-generated with comp_map.py.  DO NOT EDIT! */\n\n')
    outfile.write('#include "bytecode.h"\n\n')
    outfile.write('int python_%d_map(int id)\n' % mapver)
    outfile.write('{\n')
    outfile.write('    switch (id) {\n')
    for i in sorted(idToOpcode):
        outfile.write('    case %d: return Pyc::%s;\n' % (i, idToOpcode[i]))
    outfile.write('    default: return Pyc::PYC_INVALID_OPCODE;\n')
    outfile.write('    }\n')
    outfile.write('}\n')

    infile.close()
    outfile.close()
