"""
test_del.py -- source test pattern for 'del' statements

This source is part of the decompyle test suite.
Snippet taken from python libs's test_class.py

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

raise "This program can't be run"

print 0
a = b[5]
print 1
del a
print 2
del b[5]
print 3

del testme[1]
print 4
del testme[:]
print '4a'
del testme[:42]
print '4b'
del testme[40:42]
print 5
del testme[2:1024:10]
print '5a'
del testme[40, 41, 42]
print 6
del testme[:42, ..., :24:, 24, 100]
print 7
