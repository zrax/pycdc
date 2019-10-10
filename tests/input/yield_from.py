def gen10():
    for i in range(10):
        yield i

def gen20():
    for i in range(10, 20):
        yield i

def generator():
    yield from gen10()
    yield from gen20()
