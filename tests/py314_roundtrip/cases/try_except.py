def divide(a, b):
    try:
        return a / b
    except ZeroDivisionError:
        return 'div0'

def classify(x):
    try:
        if x < 0:
            raise ValueError('neg')
        return 'ok'
    except ValueError as e:
        return 'caught:' + str(e)
    finally:
        pass

def multi(x):
    try:
        if x == 1:
            raise KeyError('k')
        if x == 2:
            raise IndexError('i')
        return 'none'
    except KeyError:
        return 'key'
    except IndexError:
        return 'index'

def __test__():
    return [
        repr(divide(6, 3)), repr(divide(1, 0)),
        repr(classify(-1)), repr(classify(5)),
        repr(multi(1)), repr(multi(2)), repr(multi(9)),
    ]
