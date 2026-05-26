try:
     a = 1/0
except ZeroDivisionError:
     a = 1


try:
    a = 2 / 0
except (ZeroDivisionError, AssertionError) as v:
    print(v)
except Exception:
    print("there was an exception")
finally:
    b = 0
    a = 2