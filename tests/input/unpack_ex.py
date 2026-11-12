def call_unpack(func, a, b, d1, d2):
    func(*a, *b)
    func(**d1, **d2)
    func(*a, **d2)


def display_unpack(a, b, d1, d2):
    t = (*a, *b)
    l = [*a, *b]
    s = {*a, *b}
    d = {**d1, **d2}
    return t, l, s, d
