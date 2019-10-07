def try_finally():
    print 'before'
    try:
        print 'try'
    finally:
        print 'finally'
    print 'after'

def try_else_finally():
    print 'before'
    try:
        print 'try'
    except AttributeError:
        print 'except'
    else:
        print 'else'
    finally:
        print 'finally'
    print 'after'

def try_except_except():
    print 'before'
    try:
        print 'try'
    except KeyError:
        print 'except (1)'
    except:
        print 'except (2)'
    else:
        print 'else'
    print 'after'

def try_except_except_finally():
    print 'before'
    try:
        print 'try'
    except KeyError:
        print 'except (1)'
    except:
        print 'except (2)'
    else:
        print 'else'
    finally:
        print 'finally'
    print 'after'

def finally_if(a):
    print 'before'
    try:
        print 'try'
    finally:
        if a:
            print 'if'
        else:
            print 'else'
    print 'after'
