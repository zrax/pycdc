# tests from CPython (lib/test/dtracedata/call_stack.py)
def function_5(dummy, dummy2, **dummy3):
    if False:
        return 7
    return 8


def start():
    function_5(*(1, 2), **{"test": 42})


start()

# other tests to try intricated params
a = {"a": 1}
b = (1, 2)
c = {"c": 0}

def f(arg1, arg2, arg3, **kwargs):
    return 1

f(*a, *b, kwarg=0, **c) # type: ignore
