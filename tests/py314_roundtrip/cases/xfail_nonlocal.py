# XFAIL: pycdc does not emit the `nonlocal` declaration, so rebinding a
# captured variable raises UnboundLocalError after recompilation.
def make_counter():
    count = 0
    def inc():
        nonlocal count
        count = count + 1
        return count
    return inc

def __test__():
    c = make_counter()
    c(); c()
    return [repr(c())]
