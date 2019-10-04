import random

i = random.randint(0, 5)
if i is 1:
    print '%d is 1' % i
elif i < 5:
    print '%d less than 5' % i
    if i is 3:
        print '%d = 3' % i
    else:
        print '%d != 3' % i
else:
    print '%d is 5' % i
