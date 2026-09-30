"""Nested try/loop matrix: for-try-if, while-try-if, try-while-if,
try-for-if, try-in-try-inside-loop, loop-try-continue, try-finally in a
while, and sequential loop/try arrangement (loop followed by try and
vice versa).

Observation targets (Python 3.13/3.14 bytecode):
 - exception-table entries at loop depth > 0 (handler in the epilogue)
 - CHECK_EXC_MATCH chains building tuple handlers
 - POP_EXCEPT + backward jump = continue inside except
 - except + finally + continue triple region interaction
 - finally regions duplicated after the try region
 - raise-from-if guard layout (raise then dead stack-balance POP_TOP)
"""
def for_try_if(values):
    out = []
    for v in values:
        try:
            n = int(v)
        except (TypeError, ValueError):
            out.append(('err', str(v)))
            continue
        if n > 0:
            out.append(('pos', n))
    return out


def while_try_if(chunks):
    out = []
    i = 0
    while i < len(chunks):
        try:
            out.append(len(chunks[i]) // chunks[i].count('x'))
        except ZeroDivisionError:
            out.append(-1)
        i += 1
    return out


def try_while_if(n):
    seen = []
    try:
        x = n
        while x > 0:
            if x % 5 == 0:
                seen.append(x)
            x -= 1
    finally:
        seen.append('done')
    return seen


def try_for_if(items):
    total = 0
    try:
        for it in items:
            if it is None:
                raise ValueError('none item')
            total += len(it)
    except ValueError as exc:
        return ('caught', str(exc), total)
    return total


def nested_try_three(values):
    out = []
    for v in values:
        try:
            while True:
                try:
                    out.append(10 // v)
                except ZeroDivisionError:
                    out.append('div0')
                break
        except TypeError:
            out.append('type')
    return out


def loop_then_try(values, fallback):
    odds = []
    for v in values:
        if v % 2:
            odds.append(v)
    try:
        return odds[fallback]
    except IndexError:
        return 'oor'


def try_then_loop(values):
    result = []
    try:
        head = values[0]
    except IndexError:
        return ['empty']
    for v in values[1:]:
        result.append(head + v)
    return result


def while_finally(n):
    log = []
    i = 0
    while i < n:
        try:
            if i == 2:
                raise RuntimeError('at2')
        except RuntimeError:
            log.append('r' + str(i))
            i += 1
            continue
        finally:
            log.append(i)
        i += 1
    return log


def __test__():
    return [
        repr(for_try_if(['3', 'x', '7', None, '2'])),
        repr(while_try_if(['ax', 'xx', '', 'yx'])),
        repr(try_while_if(12)),
        repr(try_for_if(['ab', None, 'cdef'])),
        repr(try_for_if(['ab', 'cdef'])),
        repr(nested_try_three([2, 0, 's', 5])),
        repr(loop_then_try([1, 3, 5], 1)),
        repr(loop_then_try([1, 3, 5], 9)),
        repr(try_then_loop([10, 1, 2])),
        repr(try_then_loop([])),
        repr(while_finally(4)),
    ]
