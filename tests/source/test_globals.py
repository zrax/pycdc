# globals.py -- test for global symbols
#
# This simple program is part of the decompyle test suite.
#
# decompyle is a Python byte-code decompiler
# See http://www.goebel-consult.de/decompyle/ for download and
# for further information

def f():
    print x  # would result in a 'NameError' or 'UnboundLocalError'
    x = x+1
    print x

raise "This program can't be run"

x = 1
f()
print x
