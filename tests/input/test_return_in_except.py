def test():
    try:
        a = 1
    except ValueError:
        return 0
    except Exception:
        c = 1
    b = 3

def test1():
    a = 1
    if a:
        try:
            a = 1
        except ValueError:
            b = 2
            return 2
        except IndexError:
            b = 3
            return 3
        except Exception:
            b = 4
            return 4
        finally:
            b = 5
            return 5
        b = 1
    else:
        a = 2
    b = 3

def test2():
    a = 1
    if a:
        try:
            a = 1
        except ValueError:
            b = 2
            return 2
        except IndexError:
            b = 3
            return 3
        except Exception:
            b = 4
            return 4
        finally:
            b = 5
            return 5
    else:
        a = 2
    b = 3


def test3():
    a = 1
    if a:
        try:
            a = 1
        except ValueError:
            b = 2
        except IndexError:
            b = 3
        except Exception:
            b = 4
        c = 1
    else:
        a = 2
    b = 114


# BUG
# def test4():
#     a = 1
#     if a:
#         try:
#             a = 1
#         except ValueError:
#             b = 2
#         except IndexError:
#             b = 3
#         except Exception:
#             b = 4
#     else:
#         a = 2
#     b = 114


def test5():
    a = 1
    if a:
        try:
            a = 1
        except ValueError:
            b = 2
            return 2
        except IndexError:
            b = 3
            return 3
        except Exception:
            b = 4
            return 4
        b = 1
    else:
        a = 2
    b = 3

# BUG
# def test6():
#     a = 1
#     if a:
#         try:
#             a = 1
#         except ValueError:
#             b = 2
#             return 2
#         except IndexError:
#             b = 3
#             return 3
#         except Exception:
#             b = 4
#             return 4
#     else:
#         a = 2
#     b = 3
