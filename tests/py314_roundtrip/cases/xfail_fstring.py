# XFAIL: f-string interpolation reconstruction (NODE_JOINEDSTR) is not
# implemented; the value parts render as bare expressions.
def greet(name, score):
    return f'{name}: {score + 1}'

def __test__():
    return [repr(greet('bob', 9))]
