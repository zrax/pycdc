def squares(xs):
    for x in xs:
        yield x * x

def running_total(xs):
    total = 0
    for x in xs:
        total = total + x
        yield total

def greturn(xs):
    for x in xs:
        if x < 0:
            return 'neg'
        yield x

def __test__():
    return [
        repr(list(squares([1, 2, 3]))),
        repr(list(running_total([1, 2, 3]))),
        repr(list(greturn([1, -2, 3]))),
        repr(sum(squares(range(4)))),
    ]
