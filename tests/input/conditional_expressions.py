a = 1
result = 'even' if a % 2 == 0 else 'odd'
print(result)
# odd

a = 2
result = 'even' if a % 2 == 0 else 'odd'
print(result)
# even

a = 1
result = a * 2 if a % 2 == 0 else a * 3
print(result)
# 3

a = 2
result = a * 2 if a % 2 == 0 else a * 3
print(result)
# 4

a = 1
print('even') if a % 2 == 0 else print('odd')
# odd

a = 1

if a % 2 == 0:
    print('even')
else:
    print('odd')
# odd

a = -2
result = 'negative and even' if a < 0 and a % 2 == 0 else 'positive or odd'
print(result)
# negative and even

a = -1
result = 'negative and even' if a < 0 and a % 2 == 0 else 'positive or odd'
print(result)
# positive or odd

