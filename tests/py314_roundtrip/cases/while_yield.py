def counter(n):
    i = 0
    while i < n:
        yield i
        i = i + 1

def __test__():
    return [repr(list(counter(3)))]
