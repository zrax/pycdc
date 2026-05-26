# the compiler optimizes the dict creation using a MAP_ADD
d = {
    1: 1,
    2: 1,
    3: 1,
    4: 1,
    5: 1,
    6: 1,
    7: 1,
    8: 1,
    9: 1,
    10: 1,
    11: 1,
    12: 1,
    13: 1,
    14: 1,
    15: 1,
    16: 1,
    17: 1,
}


# the following dict comes from cpython tests (lib/test/test_compile.py)
# multiline_dict_comprehension
{x: 2 * x for x in [1, 2, 3] if (x > 0)}

# complete example with multiple conditions does not work, related to POP_JUMP_IF_FALSE
# if (x > 0  and x < 100 and x != 50)
