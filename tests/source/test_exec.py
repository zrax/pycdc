# exec.py -- source test pattern for exec statement
#
# This simple program is part of the decompyle test suite.
#
# decompyle is a Python byte-code decompiler
# See http://www.goebel-consult.de/decompyle/ for download and
# for further information

testcode = 'a = 12'

exec testcode
exec testcode in globals()
exec testcode in globals(), locals()
