## Tests from CPython tests (Lib/test/test_unpack_ex.py)
# Unpack tuple
t = (1, 2, 3)
a, *b, c = (1, 2, 3)

# Unpack list
l = [4, 5, 6]
a, *b = l
a, *b, c = [4,5,6]

#Unpack implied tuple
*a, = 7, 8, 9

# Unpack nested implied tuple
[*[*a],b] = [[7, 8, 9]]  # note for tests, another notation possible is: *(*a,),b = [[7, 8, 9]]
[*[*a]] = [[7, 8, 9]]    # note for tests, another notation possible is: *(*a,), = [[7, 8, 9]]

# Unpack string... fun!
a, *b = "one"

# Unpack long sequence
a, b, c, *d, e, f, g = range(10)

# Unpack short sequence
a, *b, c = (1, 2)

# Unpack in for statement
for a, *b, c in [(1,2,3), (4,5,6,7)]:
    print(a, b, c)

# Unpack in list
[a, *b, c] = range(5)

# Multiple targets
a, *b, c = *d, e = range(5)

# Assignment unpacking
a, b, *c = range(5)
*a, b, c = a, b, *c