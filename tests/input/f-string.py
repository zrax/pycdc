var1 = 'x'
var2 = 'y'
x = s1 = var3 = 1.23456
a = 15
some_dict = {}
some_dict[2] = 3

f''
f'''{123}'''
f'''{123}{var1}'''
f'''{123}ok'''
f'''ok{123}'''
assigned = f'''{123}'''
print(f'''{123}''')
print(f'''{123}{123}{var3}{123}''')
print(f'''{var3}''')
print(f'''{var3:4.5}''')
print(f'''f-string {123}''')
print(fr'{123}:\s+')
print(f'x{12}' * 3)
print(f'''f-string. \t\tformat value 0: {var1}, 1 (!s): {var2!s}, 2 (!r): {var2!r}, 3 (!a): {var2!a}, 4: {var3:6.3}, constant: {123}. End.''')
print('percent format %d ' % 444 + f'''f-string {123} and {var1!s}''' + f''' add another f-str {var3:2.3}''' + ' regular string ' * 2)
print(f"""'''{'single quoted string'} 'singles in f-string' {"single quote ' inside"} "doubles in f-string" {"double quoted string"} " both ' {'double quotes " inside'}'''""")
print(f'''"""{'single quoted string'} 'singles in f-string' {"single quote ' inside"} "doubles in f-string" {"double quoted string"} " both ' {'double quotes " inside'}"""''')
print(f'single quote \t\t{var1}"{var1!s}" \'{var2!a}{var3:.2f}\' """{var1!r}""" \'\'\'{var2}\'\'\'')
print(f"double quote \t\t{var1}\"{var1!s}\" '{var2!a}{var3:.2f}' \"\"\"{var1!r}\"\"\" '''{var2}'''")
print(f'{var3 * x} {var3:.2f} {var3:.5f} {x:02} {x*x:3} {x*x*x:4} {s1:>10} {a:x} {a:o} {a:e}')
print(f'''some {{braces}} {"inner literal: {braces} {{double braces}}"}''')
print(f'''f-string dict {some_dict[2]} and {{function call in expression}}: {max([1,20,3])}''')
print(f'{(lambda x: x*2)(3)}')
msg = (
    f'a {var1}'
    f'cool'
    f'multiline {var2}\n'
    f'f-string {var3}'
)
print(f'{now:%Y-%m-%d %H:%M}')