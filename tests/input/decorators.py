@staticmethod
def f():
    pass


@app.route
@cache
def g():
    return 1


@functools.lru_cache
def h(n):
    return n
