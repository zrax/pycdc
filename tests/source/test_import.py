"""
test_import.py -- source test pattern for import statements

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

import sys
import os, sys, BaseHTTPServer

import test.test_MimeWriter

from rfc822 import Message
from mimetools import Message, decode, choose_boundary
from os import *

for k, v in globals().items():
    print `k`, v
