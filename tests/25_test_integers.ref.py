"""
test_integers.py -- source test pattern for integers

This source is part of the decompyle test suite.
Snippet taken from python libs's test_class.py

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

import sys

i = 1
i = 42
i = -1
i = -42
i = sys.maxint
minint = -sys.maxint - 1
print sys.maxint
print minint
print long(minint) - 1
print
i = -2147483647
print i, repr(i)
i = i - 1
print i, repr(i)
i = -0x80000000L
print i, repr(i)
i = -0x80000001L
print i, repr(i)
