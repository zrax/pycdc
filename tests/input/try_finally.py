try:
    import sys
    try:
        print ('something else')
    finally:
        print ('...ok')
finally:
    print ('Exiting')


try:
    try:
        print("try")
    finally:
        print("finally in")
finally:
    print("finally out")
