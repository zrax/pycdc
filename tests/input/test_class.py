"""
test_class.py -- source test pattern for class definitions

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

class A:

    class A1:
        def __init__(self):
            print('A1.__init__')

        def foo(self):
            print('A1.foo')

    def __init__(self):
        print('A.__init__')

    def foo(self):
        print('A.foo')


class B:
    def __init__(self):
        print('B.__init__')

    def bar(self):
        print('B.bar')


class C(A, B):
    def foobar(self):
        print('C.foobar')


c = C()
c.foo()
c.bar()
c.foobar()
