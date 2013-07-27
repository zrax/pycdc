class X:
    def __init__(self, klass, *args, **kw):
        self.klass = klass
        self.args = args
        self.kw = kw

    def y(self):
        return self.klass(*self.args, **self.kw)