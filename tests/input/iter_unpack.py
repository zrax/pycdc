def map_with_index(func, lst):
    res = []
    for i, x in enumerate(lst):
        res.append(func(i, x))
    return res
