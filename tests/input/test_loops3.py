def loop1():
    iterable = [1, 2, 3]
    for item in iterable:
        pass

loop1()

def loop2():
    for i in range(2):
        print(i)

loop2()

def loop3():
    def loop():
        x = (1,2,3)
        l = []
        for i in x:
            l.append(i)
        return l
    
    return loop()

loop3()

def loop4():
    for i in range(3):
        for j in range(2):
            print(i*j)

loop4()

for j in [1,2,3][::-1]:
    print("hi", j)
