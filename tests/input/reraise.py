# This tests come from CPython test suite, some of them are a little bit modified.
def intricate_try_except():
    try:
        try:
            a = 1
        except AssertionError as a:
            print(a)
        except TypeError:
            raise
    except Exception as e:
        raise e
    
def reraise_example():
    try: 
        1/0
    except ZeroDivisionError: 
        raise