"""
print_to.py -- source test pattern for 'print >> ...' statements

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""
import sys

print >>sys.stdout, 1,2,3,4,5

print >>sys.stdout, 1,2,3,4,5,
print >>sys.stdout

print >>sys.stdout, 1,2,3,4,5,
print >>sys.stdout, 1,2,3,4,5,
print >>sys.stdout

print >>sys.stdout
