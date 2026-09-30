def dmin(a=1):
    return a

def d_kwargs(a, b=1, *, k=2):
    return (a, b, k)

def d_multi_kwonly(a, *, x=1, y=2, z=3):
    return (a, x, y, z)

def d_posonly(a, b=1, /, c=2):
    return (a, b, c)

def d_varargs(*args, **kwargs):
    return (args, sorted(kwargs.items()))

def __test__():
    return [
        repr(dmin()), repr(dmin(5)),
        repr(d_kwargs(1)), repr(d_kwargs(1, 2)), repr(d_kwargs(1, 2, k=3)),
        repr(d_multi_kwonly(0)), repr(d_multi_kwonly(0, y=9)),
        repr(d_posonly(1)), repr(d_posonly(1, 2, 3)),
        repr(d_varargs()), repr(d_varargs(1, 2, x=3, y=4)),
    ]
