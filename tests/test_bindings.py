import marshal

from pycdc import decompyle


def func():
    a = 6
    data = foobar(a)
    return data


body = """\
a = 6
data = foobar(a)
return data
"""


def test_marshalled_code():
    assert decompyle(marshal.dumps(func.__code__)) == body
