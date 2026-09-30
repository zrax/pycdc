class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def mag2(self):
        return self.x * self.x + self.y * self.y

class Point3(Point):
    def __init__(self, x, y, z):
        Point.__init__(self, x, y)
        self.z = z

def use():
    p = Point(3, 4)
    q = Point3(1, 2, 5)
    return (p.mag2(), q.mag2(), q.z)

def __test__():
    return [repr(use())]
