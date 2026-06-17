import json


def greet(name, greeting='hello'):
    return greeting + ', ' + name


print(greet('world', greeting='hi'))
print(json.dumps({}, ensure_ascii=False))
