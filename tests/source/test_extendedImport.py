# extendedImport.py -- source test pattern for extended import statements
#
# This simple program is part of the decompyle test suite.
#
# decompyle is a Python byte-code decompiler
# See http://www.goebel-consult.de/decompyle/ for download and
# for further information

import os, sys as System, time
import sys

from rfc822 import Message as Msg822
from mimetools import Message as MimeMsg, decode, choose_boundary as MimeBoundary

import test.test_StringIO as StringTest

for k, v in globals().items():
    print `k`, v
