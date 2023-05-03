#!/usr/bin/env python3

import sys
sys.path.append('..')
sys.path.append('obj')
import pfksqlparser
import pprint
import utils

test = 2

query = '  from SOME_TABLE \n' \
        '  select field1,field2,field3 \n' \
        '  where ((field4 ~ "^four.*five") or (4g = false)) \n' \
        '    and ((field3 > 4) or (thing like "26*"))\n' \
        '    and (field6 exists) and (field7 !exists)'

pp = pprint.PrettyPrinter(indent=2)
utils.init_trace()

if test == 1:
    utils.make_trace('memory-use-before.txt')
    for ind in range(0, 10000):
        o = pfksqlparser.parse(query)
        del o
    del ind
    utils.make_trace('memory-use-after.txt')

elif test == 2:
    print(f'query:\n  {query}')
    o = pfksqlparser.parse(query)
    print('parse tree:\n  ', end='')
    pp.pprint(o)
