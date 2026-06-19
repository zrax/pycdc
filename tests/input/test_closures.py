def outer():
    x = 10

    def inner():
        return x

    return inner


def make_adder(n):
    def adder(x):
        return x + n

    return adder


def counter():
    count = 0

    def increment():
        nonlocal count
        count += 1
        return count

    return increment


def make_multiplier(factor):
    def multiply(x):
        return x * factor

    return multiply
