def test0():
    a = 0
    while a < 10:
        a += 1
        if a == 5:
            return 10
        b = 1
    a = 1

def test1():
    a = 0
    while a < 10:
        a += 1
        if a == 5:
            return 10
    a = 1

# bug
# def test2():
#     a = 0
#     while a < 10:
#         a += 1
#         if a == 5:
#             raise Exception("123")
#     a = 1

def test3():
    a = 0
    while a < 10:
        a += 1
        if a == 5:
            raise Exception("123")
        b = 1
    a = 1

# bug
# def test4():
#     a = 0
#     while a < 10:
#         a += 1
#         if a == 5:
#             break
#     a = 1

def test5():
    a = 0
    while a < 10:
        a += 1
        if a == 5:
            break
        b = 1
    a = 1

# bug
# def test6():
#     a = 0
#     while a < 10:
#         a += 1
#         if a == 5:
#             continue
#     a = 1

def test7():
    a = 0
    while a < 10:
        a += 1
        if a == 5:
            continue
        b = 1
    a = 1
