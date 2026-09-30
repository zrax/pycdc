def slices(xs):
    return (xs[1:3], xs[::2], xs[::-1])

def walrus(xs):
    out = []
    while (n := len(xs)) > 0:
        out.append(n)
        xs = xs[:-1]
    return out

def tuples_dicts():
    t = (1, 2, 3)
    d = {'a': 1, 'b': 2}
    s = {1, 2, 3}
    return (t, d, len(s))

def __test__():
    return [
        repr(slices([0, 1, 2, 3, 4, 5])),
        repr(walrus([1, 2, 3])),
        repr(tuples_dicts()),
    ]
