class Ctx:
    def __init__(self, log, name):
        self.log = log
        self.name = name

    def __enter__(self):
        self.log.append('enter:' + self.name)
        return self.name

    def __exit__(self, *exc):
        self.log.append('exit:' + self.name)
        return False

def use_with():
    log = []
    with Ctx(log, 'a'):
        log.append('body')
    return log

def use_with_value():
    log = []
    with Ctx(log, 'b') as v:
        log.append('got:' + v)
    return log

def __test__():
    return [repr(use_with()), repr(use_with_value())]
