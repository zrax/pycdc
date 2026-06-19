def simple_gen():
    yield 1
    yield 2
    yield 3


def gen_with_for(n):
    for i in range(n):
        yield i


def gen_square(n):
    for i in range(n):
        yield i * i


def gen_filter(items, pred):
    for x in items:
        if pred(x):
            yield x


def gen_nested():
    for i in range(3):
        for j in range(3):
            yield i, j
