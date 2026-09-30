import asyncio

async def add_all(items):
    total = 0
    for it in items:
        await asyncio.sleep(0)
        total = total + it
    return total

async def nested(items):
    r = await add_all(items)
    return r * 2

def __test__():
    return [
        repr(asyncio.run(add_all([1, 2, 3]))),
        repr(asyncio.run(nested([1, 2, 3]))),
    ]
