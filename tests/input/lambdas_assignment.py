def test_logger(arg, var):
    print arg
    print var

asdf = {}
asdf['a'] = lambda x: test_logger(1, x)
asdf['b'] = lambda y: test_logger(2, y)

mult3 = filter(lambda x: x % 3 == 0, [1, 2, 3, 4, 5, 6, 7, 8, 9])
