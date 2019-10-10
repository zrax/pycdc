# Python mangles methods with a leading double-underscore with a leading
# _<classname> prefix.  This should be removed when emitting the source
#
# Valid Pythons: all

class Klass:
    def __init__(self):
        pass

    def method(self):
        pass

    def _internal_name(self):
        pass

    def __private_name(self):
        pass

    var = 1
    _internal_var = 2
    __private_var = 3

k = Klass()
k.method()
k._internal_name()

# The following is not accessible due to mangling
k.__private_name()
