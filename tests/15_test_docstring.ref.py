







"""
This is a doc string
"""

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
        foo = 'XXX22: This has to be present'
        pass        # Left for line number compatibility.  Do not emit

    def XXX13():
        pass

def Y11():
    def Y22():
        def Y33():
            pass

print __doc__
