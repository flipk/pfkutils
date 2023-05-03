
import sys
import json
import logging
import readline
#from ElasticIF import ElasticIF
import utils
from ElasticQueryBuilder import QueryBuilder

sys.path.append('common/pfksqlparser/obj')
import pfksqlparser

_debug = False
LOG_FILENAME = 'completer.log'
logging.basicConfig(filename=LOG_FILENAME,
                    level=logging.DEBUG)


def usage():
    print('usage:\n'
          '  EQ -h\n'
          '  EQ inds\n'
          '  EQ ind <ind>\n'
          '  EQ sel    (cmd prompt)\n'
          '  EQ sel "<query>"\n'
          'queries:\n'
          '  from <index> select <field>, <field>, <field> where <expr>\n'
          '    <field> can be "*"\n'
          '  <expr> : <field> <op> <value>\n'
          '  <expr> : <expr> [and|or] <expr>\n'
          '  <expr> : <field> [exists|!exists]\n'
          '  <op> : one of: < > <= >= = != ~ like\n'
          '    (note "~" means regexp)\n'
          )


def main(argv: list[str]) -> int:
    if len(argv) == 1:
        usage()
        return 0
    if argv[1] == '-h':
        usage()
        return 0

    eif = ElasticIF( .... args .... )

    if argv[1] == 'inds':

        if len(argv) != 2:
            usage()
            return 1
        return list_indices(eif)

    elif argv[1] == 'ind':

        if len(argv) != 3:
            usage()
            return 1
        return show_index(eif, argv[2])

    elif argv[1] == 'sel':

        if len(argv) == 2:
            return do_sel(eif)
        elif len(argv) == 3:
            return do_sel(eif, query=argv[2])
        else:
            usage()
            return 1

    else:
        usage()

    return 0


# reference:
#    https://elasticsearch-py.readthedocs.io/en/v8.6.1/api.html#indices
def list_indices(eif: ElasticIF) -> int:
    for indexname in eif.eif().indices.get_alias(index='*', expand_wildcards='all'):
        if indexname[0] != '.':  # skip system indices
            info = eif.eif().indices.stats(index=indexname)
            totals = info['_all']['total']
            if _debug:
                utils.dump_dict(totals, f'totalstats({indexname})')
            totaldocs = totals['docs']['count']
            bytesize = totals['store']['total_data_set_size_in_bytes']
            print({
                'index': indexname,
                'docs': totaldocs,
                'size': bytesize
            })
    return 0


def show_index(eif: ElasticIF, indexname: str) -> int:
    im = eif.eif().indices.get_field_mapping(index=indexname, fields='*')
    mappings = im[indexname]['mappings']
    for k in mappings.keys():
        if not k.startswith('_') and not k.endswith('.keyword'):
            m = mappings[k]['mapping'][k]['type']
            print({
                'index': indexname,
                'field': k,
                'type': m
            })
            if _debug:
                utils.dump_dict(mappings[k], f'mappings[{k}]')
    return 0


class SelectCompleter(object):
    _eif: ElasticIF
    _indices: list[str]
    _matches: list[str]
    _origline: str
    _begin: int
    _end: int
    _word_being_completed: str
    _words_so_far: list[str]
    _word_ind_being_completed: int

    def __init__(self, eif: ElasticIF):
        self._eif = eif
        self._indices = [ind
                         for ind in eif.eif().indices.get_alias(
                             index='*', expand_wildcards='all')
                         if ind and ind[0] != '.']
        self._matches = []
        self._origline = ''
        self._begin = 0
        self._end = 0
        self._word_being_completed = ''
        self._words_so_far = []
        self._word_ind_being_completed = 0

    def _get_fields_for_index(self, indexname: str) -> list[str]:
        im = self._eif.eif().indices.get_field_mapping(index=indexname, fields='*')
        mappings = im[indexname]['mappings']
        return [fld
                for fld in mappings.keys()
                if not fld.startswith('_') and not fld.endswith('.keyword')]

    def complete(self, text, state):
        if state == 0:
            # This is the first time for this text, so build a match list.
            self._origline = readline.get_line_buffer()
            self._begin = readline.get_begidx()
            self._end = readline.get_endidx()
            self._word_being_completed = self._origline[self._begin:self._end]
            self._words_so_far = [s
                                  for s in self._origline.split()
                                  if s not in ['(', ')']]

            if self._begin == self._end:
                if self._begin > 0 and self._origline[self._begin-1] == ',':
                    self._word_ind_being_completed = len(self._words_so_far)-1
                else:
                    self._word_ind_being_completed = len(self._words_so_far)
            else:
                self._word_ind_being_completed = len(self._words_so_far)-1

            logging.debug(f'origline: "{self._origline}" '
                          f'begin: "{self._begin}" '
                          f'end: "{self._end}" '
                          f'being_completed: "{self._word_being_completed}" '
                          f'words: "{self._words_so_far}" '
                          f'ind: {self._word_ind_being_completed}')

            if self._word_ind_being_completed == 0:
                options = ['from']
            elif self._word_ind_being_completed == 1:
                options = self._indices
            elif self._word_ind_being_completed == 2:
                options = ['select']
            elif self._word_ind_being_completed == 3:
                options = self._get_fields_for_index(
                    indexname=self._words_so_far[1])
                logging.debug(f'calculated field completions: {options}')
            elif self._word_ind_being_completed == 4:
                options = ['where']
            else:
                group_ind = (self._word_ind_being_completed - 5) % 4
                if group_ind == 0:
                    options = self._get_fields_for_index(
                        indexname=self._words_so_far[1])
                    logging.debug(f'calculated field completions: {options}')
                elif group_ind == 1:
                    options = ['>', '=', '<', '>=', '!=', '<=', '~']
                elif group_ind == 2:
                    options = []
                else:   # group_ind == 3:
                    options = ['and', 'or']

            if text:
                self._matches = [s
                                 for s in options
                                 if s and s.startswith(text)]
                logging.debug(f'text {text} matches {self._matches}')
            else:
                self._matches = options[:]
                logging.debug(f'empty string matches all options {self._matches}')

        try:
            response = self._matches[state]
        except IndexError:
            response = None
        return response


def _parse_sel(line: str) -> (dict | None):
    parsed = pfksqlparser.parse(line)
    if not parsed:
        print('failed parsing!!')
    return parsed




def _execute_sel(eif: ElasticIF, parse_tree: dict):
    indexname = parse_tree.get('table', None)
    if not indexname:
        print('no table name?')
        return
    expr = parse_tree.get('expr', None)
    if not expr:
        print('no expr?')
        return
    print('')
    print(f'parse_tree = {parse_tree}')

    fields = parse_tree.get('field_list', None)
    if fields:
        if fields[0] == '*':
            fields = None

    print('')
    qb = QueryBuilder(expr)
    print('')
    if fields:
        print(f'fields = {json.dumps(fields)}')
    print(f'{qb}')
    print('')
    if fields:
        source = False
    else:
        source = True

    count = 0
    for rec in eif.record_generator(indexname,
                                    source=source,
                                    fields=fields,
                                    size=100,
                                    body={'query': qb.query}):
        if fields:
            flds = rec.get('fields', None)
            if flds:
                print(flds)
                count += 1
        else:
            src = rec.get('_source', None)
            if src:
                print(src)
                count += 1
    print(f'documents matched: {count}')


def do_sel(eif: ElasticIF, query: str | None = None):
    if not query:
        sc = SelectCompleter(eif)
        readline.set_completer(sc.complete)
        readline.parse_and_bind('tab: complete')
        line = ''
        while line != 'stop':
            try:
                line = input('> ')
                parse_tree = _parse_sel(line)
                if parse_tree:
                    _execute_sel(eif, parse_tree)
            except EOFError:
                break
            except KeyboardInterrupt:
                break
            if line == '':
                break
    else:
        parse_tree = _parse_sel(query)
        if parse_tree:
            _execute_sel(eif, parse_tree)
    print('')
    return 0


if __name__ == '__main__':
    exit(main(sys.argv))
