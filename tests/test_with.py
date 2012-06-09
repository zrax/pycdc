with open(__file__):
    result = True

with open(__file__) as f:
    f.read()

with open(__file__) as f:
    s = f.readline()
    while s:
        s = f.readline()

with open(__file__) as f:
    result = False
    data = f.read()
    if data:
        result = True
    else:
        data = "empty"

with open(__file__) as f:
    result = None
    try:
        data = f.read()
        if data:
            result = data
        else:
            result = ""
    except:
        result = "exception"
    else:
        result += "\n"
