# XFAIL: except* (ExceptionGroup) handlers are not reconstructed.
def run(x):
    try:
        if x:
            raise ValueError('v')
        return 'ok'
    except* ValueError as eg:
        return 'group'

def __test__():
    return [repr(run(1)), repr(run(0))]
