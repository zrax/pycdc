"""
test_tuples.py -- source test pattern for tuples

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

a = (1,)
b = (2,3)
a,b = (1,2)
a,b = ( (1,2), (3,4,5) )

x = {}
try:
    x[1,2,3]
except:
    pass
x[1,2,3] = 42
print x[1,2,3]
print x[(1,2,3)]
assert x[(1,2,3)] == x[1,2,3]
del x[1,2,3]
