"""
augmentedAssign.py -- source test pattern for augmented assigns

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

raise "This program can't be run"

a = 1
b = 2
a += b; print a  # a = a+b = 3
a -= b; print a  # a = a-b = 1
a *= b; print a  # a = a*b = 2
a -= a; print a  # a = a-a = 0
a += 7*3; print a  # == 21

l= [1,2,3]
l[1] *= 3;    print l[1]; # 6
l[1][2][3] = 7
l[1][2][3] *= 3;
l[:] += [9];  print l
l[:2] += [9];  print l
l[1:] += [9];  print l
l[1:4] += [9];  print l
l += [42,43]; print l

a.value = 1
a.value += 1;
a.b.val = 1
a.b.val += 1;

l = []
for i in range(3):
    lj = []
    for j in range(3):
        lk = []
        for k in range(3):
            lk.append(0)
        lj.append(lk)
    l.append(lj)

i = j = k = 1
def f():
    global i
    i += 1
    return i

l[i][j][k] = 1
i = 1
l[f()][j][k] += 1
print i, l
