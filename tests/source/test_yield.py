from __future__ import generators

def inorder(t):
    if t:
        for x in inorder(t.left):
            yield x
        yield t.label
        for x in inorder(t.right):
            yield x

def generate_ints(n):
    for i in range(n):
        yield i*2

for i in generate_ints(5):
    print i,
print
gen = generate_ints(3)
print gen.next(),
print gen.next(),
print gen.next(),
print gen.next()
