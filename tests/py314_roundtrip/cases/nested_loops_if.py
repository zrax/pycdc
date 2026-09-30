"""Nested loop/conditional matrix: for-for-if, for-while-if, while-for-if,
while-while-if, plus a 3-deep for->if->while->for cascade with break,
continue and loop-else edge cases.

Observation targets (Python 3.13/3.14 bytecode):
 - nested back edges (JUMP_BACKWARD chains) closing inner blocks first
 - interstitial statements between an inner loop and the outer back edge
 - for/while ``else`` arms emitted as forward regions after the loop
 - ``break`` compiled to a jump past the loop-else region
"""
def for_for_if(grid):
    out = []
    for i in range(len(grid)):
        for j in range(len(grid[i])):
            if grid[i][j] % 2 == 0:
                out.append((i, j, grid[i][j]))
    return out


def for_while_if(n):
    steps = []
    for start in range(1, n + 1):
        x = start
        count = 0
        while x != 1:
            if x % 2 == 0:
                x = x // 2
            else:
                x = 3 * x + 1
            count += 1
        steps.append((start, count))
    return steps


def while_for_if(items, cap):
    idx = 0
    hits = []
    while idx < len(items):
        batch = items[idx:idx + 3]
        for v in batch:
            if v > cap:
                hits.append(v)
            elif v < 0:
                hits.append(0)
        idx += 3
    return hits


def while_while_if(n):
    a = 0
    b = 0
    total = 0
    while a < n:
        while b < n:
            if (a * b) % 3 == 0:
                total += a + b
            b += 1
        a += 1
        b = a
    return total


def three_deep(n):
    acc = []
    for i in range(n):
        if i % 2 == 0:
            j = i
            while j > 0:
                for k in range(j % 3 + 1):
                    if k == 1:
                        acc.append((i, j, k))
                        break
                else:
                    acc.append((i, j, 'else'))
                j -= 1
    return acc


def find_first_even(rows):
    for row in rows:
        for v in row:
            if v % 2 == 0:
                return v
    else:
        return None
    return None


def __test__():
    grid = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
    return [
        repr(for_for_if(grid)),
        repr(for_while_if(6)),
        repr(while_for_if([10, 2, 30, -4, 50, 6, 70], 25)),
        repr(while_while_if(4)),
        repr(three_deep(4)),
        repr(find_first_even([[1, 3], [5, 6, 7]])),
        repr(find_first_even([[1, 3], [5, 7]])),
    ]
