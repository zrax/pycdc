def evens(xs):
    return [x for x in xs if x % 2 == 0]

def __test__():
    return [repr(evens([1, 2, 3, 4]))]
