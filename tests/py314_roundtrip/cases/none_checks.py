def is_none(x):
    if x is None:
        return 'none'
    return 'some'

def not_none(x):
    if x is not None:
        return x + 1
    return -1

def coalesce(a, b):
    if a is None:
        return b
    return a

def first_not_none(xs):
    for x in xs:
        if x is not None:
            return x
    return None

def __test__():
    return [
        repr(is_none(None)), repr(is_none(0)),
        repr(not_none(None)), repr(not_none(41)),
        repr(coalesce(None, 'fb')), repr(coalesce('a', 'b')),
        repr(first_not_none([None, None, 3, 4])),
    ]
