"""
test_loops2.py -- source test pattern for loops (CONTINUE_LOOP)

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

# This is a seperate test pattern, since 'continue' within 'try'
# was not allowed till Python 2.1

for term in args:
    try:
        print
        continue
        print
    except:
        pass
