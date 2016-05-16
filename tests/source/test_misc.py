# slices.py -- source test pattern for slices
#
# This simple program is part of the decompyle test suite.
# Snippet taken from python libs's test_class.py
#
# decompyle is a Python byte-code decompiler
# See http://www.goebel-consult.de/decompyle/ for download and
# for further information

raise "This program can't be run"

class A:
    def __init__(self, num):
        self.num = num
    def __repr__(self):
        return str(self.num)

b = []
for i in range(10):
    b.append(A(i))

for i in  ('CALL_FUNCTION', 'CALL_FUNCTION_VAR',
           'CALL_FUNCTION_VAR_KW', 'CALL_FUNCTION_KW'):
    print i, '\t', len(i), len(i)-len('CALL_FUNCTION'),
    print (len(i)-len('CALL_FUNCTION')) / 3, 
    print i[len('CALL_FUNCTION'):]

p2 = (0, 0, None)
if p2[2]:
    print 'has value'
else:
    print ' no value'
