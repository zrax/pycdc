def f():
    print x
    x = x + 1
    print x

raise "This program can't be run"

x = 1
f()
print x
