def x0():
    pass

def x1(arg1):
    pass

def x2(arg1, arg2):
    pass

def x3a(*args):
    pass

def x3b(**kwargs):
    pass

def x3c(*args, **kwargs):
    pass

def x4a(foo, bar = 1, bla = 2, *args):
    pass

def x4b(foo, bar = 1, bla = 2, **kwargs):
    pass

def x4c(foo, bar = 1, bla = 2, *args, **kwargs):
    pass

def x5a(*, bar):
    pass

def x5b(*, bar = 1):
    pass

def x5c(*, bar = 1, **kwargs):
    pass

def x6a(foo, *, bar):
    pass

def x6b(foo, *, bar = 1):
    pass

def x6c(foo = 1, *, bar):
    pass

def x6d(foo = 1, *, bar = 2):
    pass

def x7a(foo, *, bar, **kwargs):
    pass

def x7b(foo, *, bar = 1, **kwargs):
    pass

def x7c(foo = 1, *, bar, **kwargs):
    pass

def x7d(foo = 1, *, bar = 2, **kwargs):
    pass
