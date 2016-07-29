"""
test_tuple_params.py -- source test pattern for formal parameters of type tuple

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

def A(a,b,(x,y,z),c):
    pass

def B(a,b=42,(x,y,z)=(1,2,3),c=17):
    pass

def C((x,y,z)):
    pass

def D((x,)):
    pass

def E((x)):
    pass
