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

maplist = [ 'python_10', 'python_11', 'python_13', 'python_14', 'python_15',
            'python_16', 'python_20', 'python_21', 'python_22', 'python_23',
            'python_24', 'python_25', 'python_26', 'python_27', 'python_30',
            'python_31' ]

for mapfile in maplist:
    infile = open(mapfile + '.map', 'rt')
    outfile = open(mapfile + '.cpp', 'wb')
    with infile and outfile:
        idToOpcode = {}
        opcodeToId = {}
        for ln in infile.readlines():
            fileid, code = ln.split()
            idToOpcode[fileid] = code
            opcodeToId[code] = fileid

        outfile.write('/* This file was auto-generated with comp_map.py.  DO NOT EDIT! */\n\n')
        outfile.write('#include "../bytecode.h"\n\n')
        outfile.write('int ' + mapfile + '_map(int id)\n')
        outfile.write('{\n')
        outfile.write('    switch (id) {\n')
        for i in idToOpcode:
            outfile.write('    case ' + i + ': return Pyc::' + idToOpcode[i] + ';\n')
        outfile.write('    default: return Pyc::PYC_INVALID_OPCODE;\n')
        outfile.write('    }\n')
        outfile.write('}\n\n')
        outfile.write('int ' + mapfile + '_unmap(int id)\n')
        outfile.write('{\n')
        outfile.write('    switch (id) {\n')
        for i in opcodeToId:
            outfile.write('    case Pyc::' + i + ': return ' + opcodeToId[i] + ';\n')
        outfile.write('    default: return -1;\n')
        outfile.write('    }\n')
        outfile.write('}\n')
