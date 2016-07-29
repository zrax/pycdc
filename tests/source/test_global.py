"""
test_global.py -- source test pattern for 'global' statement

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

i = 1; j = 7
def a():
    def b():
        def c():
            k = 34
            global i
            i = i+k
        l = 42
        c()
        global j
        j = j+l
    b()
    print i, j # should print 35, 49

a()
print i, j
