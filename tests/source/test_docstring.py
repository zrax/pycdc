# docstring.py -- source test pattern for doc strings
#
# This simple program is part of the decompyle test suite.
#
# decompyle is a Python byte-code decompiler
# See http://www.goebel-consult.de/decompyle/ for download and
# for further information

'''
This is a doc string
'''

def Doc_Test():
	"""This has to be present"""

class XXX:
	def __init__(self):
		"""__init__: This has to be present"""
		self.a = 1

		def XXX22():
			"""XXX22: This has to be present"""
			pass

	def XXX11():
		"""XXX22: This has to be present"""
		pass

	def XXX12():
		foo = """XXX22: This has to be present"""
		pass

	def XXX13():
		pass

def Y11():
	def Y22():
		def Y33():
			pass

print __doc__
