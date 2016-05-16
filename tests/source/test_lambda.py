# lambda.py -- source test pattern for lambda functions
#
# This simple program is part of the decompyle test suite.
#
# decompyle is a Python byte-code decompiler
# See http://www.goebel-consult.de/decompyle/ for download and
# for further information

palette = map(lambda a: (a,a,a), range(256))
palette = map(lambda (r,g,b): chr(r)+chr(g)+chr(b), palette)
palette = map(lambda r: r, palette)

palette = lambda (r,g,b,): r
palette = lambda (r): r
palette = lambda r: r
palette = lambda (r): r, palette
