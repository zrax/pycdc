# XFAIL: `match` statement (MATCH_CLASS/MATCH_SEQUENCE) is not supported.
def describe(x):
    match x:
        case [a, b]:
            return 'pair'
        case Point(x=0, y=0):
            return 'origin'
        case _:
            return 'other'

class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y

def __test__():
    return [repr(describe([1, 2])), repr(describe(0))]
