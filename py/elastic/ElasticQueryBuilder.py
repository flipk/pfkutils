#!/usr/bin/env python3

import sys
sys.path.append('pfksqlparser/obj')
import pfksqlparser
import pprint
import json

# "wildcard": {
#     "specstring": { "value": "29.*" }
# }
# "regexp": {
#     "specstring": {
#         "value": "29\\.[34569].*",
#         "flags": "ALL"
#     }
# }
# "bool": {
#     "must": [  # "AND" is spelled "must"
#         { "regexp": {
#                 "specstring": {
#                     "value": "29\\.[34569].*",
#                     "flags": "ALL"
#                 }
#             }
#         }, {
#             "bool": {
#                 "should": [  # "OR" is spelled "should"
#                     {"term": {"5g": True}},
#                     {"term": {"4g": True}}
#                 ]
#             }
#         }
#     ]
# }
# "bool": {
#     "must": {
#         "term": {
#             "html_status": op
#         }
#     }
# }
# "range": { "html_time": { "gt": 0 } }
# "bool": {
#     "must": { "term": {"user.id": "kimchy"} },
#     "filter": { "term": {"tags": "production"} },
#     "must_not": {
#         "range": { "age": {"gte": 10, "lte": 20} }
#     },
#     "should": [
#         {"term": {"tags": "env1"}},
#         {"term": {"tags": "deployed"}}
#     ],
#     "minimum_should_match": 1
# }

# field compare ops:
#   { "wildcard": { "FIELDNAME": { "value":  VALUE }}}
#   { "regexp": { "FIELDNAME": { "value":  VALUE, "flags": "ALL" }}}
#   { "term": { "FIELDNAME": VALUE }}
#   { "range": { "FIELDNAME": { "gt": VALUE }}}
#      legal range comparators : gt gte lt lte
#
#  { "bool": { "must"    : {  <all the ANDs>   },
#              "should"  : {  <all the ORs>    },
#              "must_not": {  <all the NOTs ?> },  # this tool doesn't use
#              "filter"  : {  <also ANDs?>     }}} # this tool doesn't use

class QueryBuilder:
    query: dict

    def __init__(self, expr: dict):
        self.query = self._eval_expr(expr)

    def __str__(self):
        return f'query = {json.dumps(self.query)}'

    def _eval_expr(self, subexpr: dict) -> dict | None:
        ret = {}
        t = subexpr.get('type', None)
        if not t:
            print(f'ERROR NO TYPE: {subexpr}')
            return None

        if t == 'compare':
            c = subexpr.get('compare', None)
            if c:
                op = c.get('op', None)
                fld = c.get('field', None)
                val = c.get('value', None)
                print(f'compare field {fld} op {op} val {val}')
                if op == 'eq':
                    ret['term'] = {fld: val}
                elif op == 'neq':
                    # noinspection PyTypedDict
                    ret['bool'] = {'must_not': {'term': {fld: val}}}
                elif op in ['lt', 'gt', 'lte', 'gte']:
                    ret['range'] = {fld: {op: val}}
                elif op == 'regex':
                    ret['regexp'] = {fld: {'value': val, 'flags': 'all'}}
                elif op == 'like':
                    ret['wildcard'] = {fld: {'value': val}}

        elif t == 'conj':
            c = subexpr.get("conj", None)
            if c:
                conj = c.get('conj', None)
                left = c.get('left', None)
                right = c.get('right', None)
                print(f'conjunction: {conj}')
                if left and right and conj:
                    lret = self._eval_expr(left)
                    rret = self._eval_expr(right)
                    if conj == 'and':
                        # noinspection PyTypedDict
                        ret['bool'] = {'must': [lret, rret]}
                    elif conj == 'or':
                        # noinspection PyTypedDict
                        ret['bool'] = {'should': [lret, rret]}

        elif t == 'exist':
            c = subexpr.get('exist', None)
            if c:
                field = c.get('field', None)
                flag = c.get('value', False)
                if field:
                    print(f'field {field} exists flag {flag}')
                    if flag:
                        ret['exists'] = {'field': field}
                    else:
                        ret['bool'] = {
                            'must_not': {
                                'exists': {'field': field}
                            }
                        }

        print(f'ret = {ret}')
        return ret


def main():
    pp = pprint.PrettyPrinter(indent=2)

    query = '  from SOME_TABLE \n' \
            '  select field1,field2,field3 \n' \
            '  where ((field4 ~ "^four.*five") or (4g = false)) \n' \
            '    and ((field3 > 4) or (thing like "26*"))\n' \
            '    and (field6 exists) and (field7 !exists)'

    print(f'query:\n{query}')
    parse_tree = pfksqlparser.parse(query)
    print('parse_tree:')
    pp.pprint(parse_tree)
    expr = parse_tree.get('expr', None)
    if not expr:
        print('no expr?')
        return
    print('')
    print('--parsing--')
    qb = QueryBuilder(expr)
    print('--parsing done--')
    print('')
    pp.pprint(qb.query)
    print('')

if __name__ == '__main__':
    exit(main())
