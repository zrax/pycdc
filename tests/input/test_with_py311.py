with open(__file__):
    result = True

with open(__file__) as f:
    result = f.read()

with open(__file__) as f:
    if f:
        result = True
    else:
        result = False

with open(__file__) as a:
    with open(__file__) as b:
        result = a.read() + b.read()

with open(__file__) as f:
    result = f.read()
result = len(result)


def read_file():
    with open(__file__) as fh:
        data = fh.read()
    return data
