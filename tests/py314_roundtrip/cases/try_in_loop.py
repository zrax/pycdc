"""Try/except inside a loop with a deferred handler (Python 3.14 layout).

The 3.14 compiler places the exception handler after the loop body
(epilogue region), exercising the deferred-mounting of the container:
 - try body statements
 - statements between the try and the loop back edge
 - continue from inside the except handler
 - a raise-guard if inside the loop followed by an explicit return
"""
import random


def main():
    total = 0
    skipped = []
    values = ['1', '2', 'x', '4']
    for v in values:
        try:
            n = int(v)
        except (TypeError, ValueError):
            skipped.append(v)
            continue
        if n < 0:
            raise ValueError('negative')
        total += n
    return total, skipped


def guard(data):
    for item in data:
        if not item.startswith(b'ok'):
            raise RuntimeError('integrity check failed')
        return len(item)
    return 0


def __test__():
    return [
        repr(main()),
        repr(guard([b'ok', b'okay'])),
    ]


if __name__ == '__main__':
    print(__test__())
