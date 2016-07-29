# expressions.py -- source test pattern for expressions
#
# This simple program is part of the decompyle test suite.
#
# decompyle is a Python byte-code decompiler
# See http://www.goebel-consult.de/decompyle/ for download and
# for further information

def _lsbStrToInt(str):
        return ord(str[0]) + \
               (ord(str[1]) << 8) + \
               (ord(str[2]) << 16) + \
               (ord(str[3]) << 24)
