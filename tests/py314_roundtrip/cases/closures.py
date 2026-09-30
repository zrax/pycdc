def make_adder(base):
    def add(x):
        return x + base
    return add

def make_mul(a, b):
    def mul(x):
        return x * a + b
    return mul

def outer_tag(tag):
    def wrap(s):
        return tag + s + tag
    return wrap

def __test__():
    return [
        repr(make_adder(10)(5)),
        repr(make_mul(3, 1)(7)),
        repr(outer_tag('<')('hi')),
        repr([make_adder(i)(i) for i in range(3)]),
    ]
