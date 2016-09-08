"""
test_global.py -- source test pattern for 'global' statement

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

i = 1
j = 7

def a():
    def b():
        global j

        def c():
            global i
            k = 34
            i = i + k

        l = 42
        c()
        j = j + l

    b()
    print i, j

a()
print i, j
