a = [y, z] = x = (k1, k2, k3) = [] = c = myfunc(x) + 3

x = y = g = {keyA: X} 

global store_global
Gx = Gy = Gz = Gq1
Gx = [Gy, Gz] = Gq2
a = b = store_global = c

def func_with_global():
    global Gx, Gy, Gz, Gq
    Gx = Gy = Gz = Gq

y = store_subscr[0] = x
a[0] = b[x] = c[3] = D[4]
a[0] = (b[x], c[3]) = D[4]
a[0] = Q = [b[x], c[3]] = F = D[4]
q = v = arr[a:b:c] = x

class store_attr1:
    def __init__(self, a,b,c):
        self.a = self.b = self.c = x
        self.d = y

class store_attr2:
    def __init__(self, a,b,c): self.a = (self.b, self.c) = x

a.b = c.d = e.f + g.h

def store_deref():
    a = I
    a = b = c = R1
    a = (b, c) = R2
    def store_fast():
        x = a
        y = b
        z = c
        p = q = r = s
        p = [q, r] = s
