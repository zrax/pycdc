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

maplist = [ 10, 11, 13, 14, 15, 16,
            20, 21, 22, 23, 24, 25, 26, 27,
            30, 31, 32, 33, 34 ]

for mapver in maplist:
    infile = open('python_%d.map' % mapver, 'rt')
    outfile = open('python_%d.cpp' % mapver, 'wt')

    idToOpcode = {}
    opcodeToId = {}
    for ln in infile.readlines():
        fileid, code = ln.split()
        idToOpcode[int(fileid)] = code
        opcodeToId[code] = int(fileid)

    outfile.write('/* This file was auto-generated with comp_map.py.  DO NOT EDIT! */\n\n')
    outfile.write('#include "../bytecode.h"\n\n')
    outfile.write('int python_%d_map(int id)\n' % mapver)
    outfile.write('{\n')
    outfile.write('    switch (id) {\n')
    for i in sorted(idToOpcode):
        outfile.write('    case %d: return Pyc::%s;\n' % (i, idToOpcode[i]))
    outfile.write('    default: return Pyc::PYC_INVALID_OPCODE;\n')
    outfile.write('    }\n')
    outfile.write('}\n\n')
    outfile.write('int python_%d_unmap(int id)\n' % mapver)
    outfile.write('{\n')
    outfile.write('    switch (id) {\n')
    for i in sorted(opcodeToId):
        outfile.write('    case Pyc::%s: return %d;\n' % (i, opcodeToId[i]))
    outfile.write('    default: return -1;\n')
    outfile.write('    }\n')
    outfile.write('}\n')

    infile.close()
    outfile.close()
