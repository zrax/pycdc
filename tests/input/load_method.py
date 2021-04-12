class C:
    def test1(self):
        a = 1

    def test2(self, x, y, z):
        a = x * y + z

    # @staticmethod -- TODO: Fix decorators
    def testS():
        a = 3

c = C()
c.test1()
c.test2(42, 5, -1)
C.testS()
