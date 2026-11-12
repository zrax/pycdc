while x:
    pass

def f(t):
    while t > 10:
        t = 1
    print(t)

def g(n):
    while n:
        if n & 1:
            n -= 1
        n //= 2
