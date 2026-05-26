b = {"b": 1}
# unpack of dict
dict(**{"c": 2}, **b)
# add arguments in plus of unpack
dict(a="a", **{"c": 2}, **b)
