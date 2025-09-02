def outer():
    x = 0
    def inner():
        nonlocal x
        x += 1
        print(x)
    return inner
