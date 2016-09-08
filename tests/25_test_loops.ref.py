"""
test_loops.py -- source test pattern for loops

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

for i in range(10):
    if i == 3:
        continue
    if i == 5:
        break
    print i,
else:
    print 'Else'
print

for i in range(10):
    if i == 3:
        continue
    print i,
else:
    print 'Else'

i = 0
while i < 10:
    i = i + 1
    if i == 3:
        continue
    if i == 5:
        break
    print i,
else:
    print 'Else'
print

i = 0
while i < 10:
    i = i + 1
    if i == 3:
        continue
    print i,
else:
    print 'Else'
