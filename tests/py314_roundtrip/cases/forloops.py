def flatten(xss):
    out = []
    for xs in xss:
        for x in xs:
            out.append(x)
    return out

def table(n, m):
    rows = []
    for i in range(n):
        row = []
        for j in range(m):
            row.append(i * j)
        rows.append(row)
    return rows

def __test__():
    return [
        repr(flatten([[1, 2], [3], []])),
        repr(table(3, 4)),
    ]
