try:
    import sys
    try:
        print ('something else')
    except AssertionError:
        print ('...failed')
except ImportError:
    print ('Oh Noes!')

try:
    try:
        print("try")
    except:
        print("except in")
except:
    print("except out")
