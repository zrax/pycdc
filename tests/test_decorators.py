@staticmethod
def square(n):
    return n * n

# From: http://wiki.python.org/moin/PythonDecoratorLibrary
def synchronized(lock):
    """Synchronization decorator."""

    def wrap(f):
        def new_function(*args, **kw):
            lock.acquire()
            try:
                return f(*args, **kw)
            finally:
                lock.release()
        return new_function
    return wrap

from threading import Lock
cache_lock = Lock()

class Cache(object):
    def __init__(self):
        self._name = 'default'

    @classmethod
    @synchronized(cache_lock)
    def cache(cls):
        return cls()

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, new_name):
        self._name = new_name
