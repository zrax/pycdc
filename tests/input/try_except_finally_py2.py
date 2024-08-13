try:
    import sys
    try:
        print 'something else'
    except AssertionError:
        print '...failed'
except ImportError:
    print 'Oh Noes!'
finally:
    print 'Exiting'
