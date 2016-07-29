"""
test_import_as.py -- source test pattern for 'import .. as 'statements

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

import sys as SYS
import os as OS, sys as SYSTEM, BaseHTTPServer as HTTPServ

import test.test_MimeWriter as Mime_Writer

from rfc822 import Message as MSG
from mimetools import Message as mimeMsg, decode, \
     choose_boundary as mimeBoundry

print '---' * 20

for k, v in globals().items():
    print k, repr(v)
