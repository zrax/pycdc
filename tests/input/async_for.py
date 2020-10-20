async def a(b, c):
    async for b in c:
        pass

async def a(b, c):
    async for b in c:
        continue

async def a(b, c):
    async for b in c:
        break

async def time_for_some_fun():
    async for (x, y) in myfunc(c):
        print(123)
        if (x == 3):
            print('something')
            break

        for i in regular_for:
            var1 = var2 + var3
            async for x1 in print:
                print('test LOAD_GLOBAL')
                async for x2 in inner:
                    for x3 in regular:
                        async for x4 in inner2:
                            async for x5 in inner3:
                                async for x6 in inner4:
                                    print('ridiculous nesting')

    async for (q, w, e, r) in qwer:
        u = 1 + 2 + 3

    async for x4 in inner2:
        async for x5 in inner3:
            pass
        
    print('outside loop')

print ('outside function')

# The following will LOAD_METHOD, not GET_AITER or GET_ANEXT.
# test.__anext__(iter)
# test.__aiter__(iter)
