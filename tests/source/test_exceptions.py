import dis

def x11():
    try:
        a = 'try except'
    except:
        a = 2
    b = '--------'


def x12():
    try:
        a = 'try except else(pass)'
    except:
        a = 2
    b = '--------'


def x13():
    try:
        a = 'try except else(a=3)'
    except:
        a = 2
    else:
        a = 3
    b = '--------'


def x21():
    try:
        a = 'try KeyError'
    except KeyError:
        a = 8
    b = '--------'


def x22():
    try:
        a = 'try (IdxErr, KeyError) else(pass)'
    except (IndexError, KeyError):
        a = 8
    b = '--------'


def x23():
    try:
        a = 'try KeyError else(a=9)'
    except KeyError:
        a = 8
    else:
        a = 9
    b = '--------'


def x31():
    try:
        a = 'try KeyError IndexError'
    except KeyError:
        a = 8
    except IndexError:
        a = 9
    b = '--------'


def x32():
    try:
        a = 'try KeyError IndexError else(pass)'
    except KeyError:
        a = 8
    except IndexError:
        a = 9
    b = '--------'


def x33():
    try:
        a = 'try KeyError IndexError else(a=9)'
    except KeyError:
        a = 8
    except IndexError:
        a = 9
    else:
        a = 9
    b = '#################'


def x41():
    if (a == 1):
        a = 1
    elif (b == 1):
        b = 1
    else:
        c = 1
    b = '#################'


def x42():
    if (a == 1):
        a = 1
    elif (b == 1):
        b = 1
    else:
        c = 1
    xxx = 'mmm'

if (__name__ == '__main__'):
    dis.dis(xx)
