import sys

width = 80
height = 24
inner_l = int((width - 60) / 2)
inner_r = 61 + inner_l

sys.stderr.write('\x1b?3l')
sys.stderr.write('\x1b[H')
sys.stderr.write('\x1b#8')
sys.stderr.write('\x1b[9;%dH' % inner_l)
sys.stderr.write('\x1b[1J')
sys.stderr.write('\x1b[18;60H')
sys.stderr.write('\x1b[0J')
sys.stderr.write('\x1b[1K')
sys.stderr.write('\x1b[9;%dH' % inner_r)
sys.stderr.write('\x1b[0K')

i = 10
while i <= 16:
    sys.stderr.write('\x1b[%d;%dH' % (i, inner_l))
    sys.stderr.write('\x1b[1K')
    sys.stderr.write('\x1b[%d;%dH' % (i, inner_r))
    sys.stderr.write('\x1b[0K')
    i += 1

sys.stderr.write('\x1b[17;30H')
sys.stderr.write('\x1b[2K')

i = 1
while i <= width:
    sys.stderr.write('\x1b[%d;%df' % (height, i))
    sys.stderr.write('*')
    sys.stderr.write('\x1b[%d;%df' % (1, i))
    sys.stderr.write('*')
    i += 1

sys.stderr.write('\x1b[2;2H')

i = 2
while i < height:
    sys.stderr.write('+')
    sys.stderr.write('\x1b[1D')
    sys.stderr.write('\x1bD')
    i += 1

sys.stderr.write('\x1b[%d;%dH' % (height - 1, width - 1))

i = height - 1
while i > 1:
    sys.stderr.write('+')
    sys.stderr.write('\x1b[1D')
    sys.stderr.write('\x1bM')
    i -= 1

sys.stderr.write('\x1b[2;1H')

i = 2
while i < height:
    sys.stderr.write('*')
    sys.stderr.write('\x1b[%d;%dH' % (i, width))
    sys.stderr.write('*')
    sys.stderr.write('\x1b[10D')
    if i < 10:
        sys.stderr.write('\x1bE')
    else:
        sys.stderr.write('\n')
    i += 1

sys.stderr.write('\x1b[2;10H')
sys.stderr.write('\x1b[42D')
sys.stderr.write('\x1b[2C')

i = 3
while i < width - 1:
    sys.stderr.write('+')
    sys.stderr.write('\x1b[0C')
    sys.stderr.write('\x1b[2D')
    sys.stderr.write('\x1b[1C')
    i += 1

sys.stderr.write('\x1b[%d;%dH' % (height - 1, inner_r - 1))
sys.stderr.write('\x1b[42C')
sys.stderr.write('\x1b[2D')

i = width - 2
while i > 2:
    sys.stderr.write('+')
    sys.stderr.write('\x1b[1D')
    sys.stderr.write('\x1b[1C')
    sys.stderr.write('\x1b[0D')
    sys.stderr.write('\x08')
    i -= 1

sys.stderr.write('\x1b[10;%dH' % (2 + inner_l))

i = 10
while i <= 15:
    j = 2 + inner_l
    while j < inner_r - 1:
        sys.stderr.write(' ')
        j += 1
    sys.stderr.write('\x1b[1B')
    sys.stderr.write('\x1b[58D')
    i += 1

try:
    input('')
except SyntaxError:
    pass
