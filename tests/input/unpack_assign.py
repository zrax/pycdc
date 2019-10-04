x = (1, 2, 3)
a, b, c = x
#c, b, a = (a, b, c)        # This gets optimized by newer Python compilers...

x = (1,)
a, = x
#c, = (a,)                  # This gets optimized by newer Python compilers...
