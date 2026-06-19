# 3.10+ structural pattern matching (match/case) reconstruction.

def describe_point(command):
    match command:
        case Point(x, y):
            return f'point {x},{y}'
        case Rect(w, h):
            return f'rect {w}x{h}'
        case Wrapper(_):
            return 'wrapped'
        case _:
            return 'unknown'


def classify(value):
    match value:
        case 0:
            result = 'zero'
        case 1:
            result = 'one'
        case 'hello':
            result = 'greeting'
        case _:
            result = 'other'
    return result
