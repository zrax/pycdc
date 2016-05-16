# test_functions.py -- source test pattern for functions
#
# This source is part of the decompyle test suite.
#
# decompyle is a Python byte-code decompiler
# See http://www.goebel-consult.de/decompyle/ for download and
# for further information

def x0():
    pass

def x1(arg1):
    pass

def x2(arg1,arg2):
    pass

def x3a(*args):
    pass

def x3b(**kwargs):
    pass

def x3c(*args, **kwargs):
    pass

def x4a(foo, bar=1, bla=2, *args):
    pass

def x4b(foo, bar=1, bla=2, **kwargs):
    pass

def x4c(foo, bar=1, bla=2, *args, **kwargs):
    pass

def func_with_tuple_args((a,b)):
    print a
    print b

def func_with_tuple_args2((a,b), (c,d)):
    print a
    print c

def func_with_tuple_args3((a,b), (c,d), *args):
    print a
    print c

def func_with_tuple_args4((a,b), (c,d), **kwargs):
    print a
    print c

def func_with_tuple_args5((a,b), (c,d), *args, **kwargs):
    print a
    print c

def func_with_tuple_args6((a,b), (c,d)=(2,3), *args, **kwargs):
    print a
    print c
