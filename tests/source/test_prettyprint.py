"""
test_prettyprint.py --	source test pattern for tesing the prettyprint
			funcionality of decompyle

This source is part of the decompyle test suite.

decompyle is a Python byte-code decompiler
See http://www.goebel-consult.de/decompyle/ for download and
for further information
"""

import pprint

aa = 'aa'

dict0 = {
    'a': 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
    'b': 1234,
    'd': aa,
    aa: aa
    }


dict = {
    'a': 'aaa',
    'b': 1234,
    'c': { 'ca': 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
           'cb': 1234,
           'cc': None
           },
    'd': aa,
    aa: aa,
    'eee': { 'ca': 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
           'cb': 1234,
           'cc': None
           },
    'ff': aa,
    }
list1 = [ '1aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
          aa,
          '1bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
          '1ccccccccccccccccccccccccccccccccccccccccccc' ]
list2 = [ '2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
          [ '22aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
            aa,
            '22bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
            '22ccccccccccccccccccccccccccccccccccccccccccc' ],
          'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
          'ccccccccccccccccccccccccccccccccccccccccccc' ]
tuple1 = ( '1aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
           aa,
           '1bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
           '1ccccccccccccccccccccccccccccccccccccccccccc' )
tuple2 = ( '2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
           ( '22aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
             aa,
             '22bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
             '22ccccccccccccccccccccccccccccccccccccccccccc' ),
           'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
           'ccccccccccccccccccccccccccccccccccccccccccc' )

def funcA():
    dict = {
        'a': 'aaa',
        'b': 1234,
        'c': { 'ca': 'aaa',
               'cb': 1234,
               'cc': None
               },
        'd': aa,
        aa: aa
        }
    list1 = [ '1aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
              '1bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
              aa,
              '1ccccccccccccccccccccccccccccccccccccccccccc' ]
    list2 = [ '2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
              [ '22aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
                aa,
                '22bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
                '22ccccccccccccccccccccccccccccccccccccccccccc' ],
              'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
              'ccccccccccccccccccccccccccccccccccccccccccc' ]
    tuple1 = ( '1aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
               '1bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
               aa,
               '1ccccccccccccccccccccccccccccccccccccccccccc' )
    tuple2 = ( '2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
               ( '22aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
                 aa,
                 '22bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
                 '22ccccccccccccccccccccccccccccccccccccccccccc' ),
               'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
               'ccccccccccccccccccccccccccccccccccccccccccc' )

    def funcAB():
        dict = {
            'a': 'aaa',
            'b': 1234,
            'c': { 'ca': 'aaa',
                   'cb': 1234,
                   'cc': None
                   },
            'd': aa,
            aa: aa
            }
        list1 = [ '1aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
                  '1bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
                  '1ccccccccccccccccccccccccccccccccccccccccccc' ]
        list2 = [ '2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
                  [ '22aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
                    '22bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
                    '22ccccccccccccccccccccccccccccccccccccccccccc' ],
                  'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
                  'ccccccccccccccccccccccccccccccccccccccccccc' ]
        tuple1 = ( '1aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
                   '1bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
                   '1ccccccccccccccccccccccccccccccccccccccccccc' )
        tuple2 = ( '2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
                   ( '22aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
                     '22bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
                     '22ccccccccccccccccccccccccccccccccccccccccccc' ),
                   'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
                   'ccccccccccccccccccccccccccccccccccccccccccc' )

pprint.pprint(dict0)
print
pprint.pprint(dict)
print

pprint = pprint.PrettyPrinter(indent=2)
pprint.pprint(dict0)
print
pprint.pprint(dict)
print

pprint.pprint(list1)
print
pprint.pprint(list2)
