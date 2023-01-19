def yield1():
	yield 123

def return1():
	return 321

def yieldFrom1():
	yield from yield1()

a = next(yield1())
b = return1()
c = next(yieldFrom1())

print(f"A: {a} / B: {b} / C: {c}")