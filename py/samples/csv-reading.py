#!/usr/bin/env python3

import collections
import sys
import csv
import Stats

# if you have a csv file that looks like this:
#   one,two,three
#   1,2,3
#   4,5,6
#   7,8,9
#
# it will print:
# {'one': '1', 'two': '2', 'three': '3'}
# {'one': '4', 'two': '5', 'three': '6'}
# {'one': '7', 'two': '8', 'three': '9'}


# things like Iterable were moved in 3.8 to collections.abc.
try:
    collectionsAbc = collections.abc
except AttributeError:
    collectionsAbc = collections


def csv_file_generator(fullpath: str) -> collectionsAbc.Iterable:
    """
    this assumes the first row is column names. it returns a dict
    where the column headers are the property names.
    :param fullpath: str: the path to the CSV file.
    :return dict: a dict where the property names are the column headers.
    """
    stats = Stats.Stats()
    total_rows = stats.stat('total_rows')
    # notvalid = stats.stat('notvalid')
    try:
        with open(fullpath, mode='r', newline='', encoding='utf8') as csvf:
            rdr = csv.reader(csvf, dialect='excel')
            first = True
            hdr = None
            for row in rdr:
                if first:
                    hdr = row
                    first = False
                else:
                    total_rows.peg()
                    cols = {}
                    for ind in range(len(hdr)):
                        cols[hdr[ind]] = row[ind]
                    yield cols

    # NOTE the normal behavior of a generator is to throw
    # a GeneratorExit exception when the generator is complete.
    # GeneratorExit is derived from BaseExeption, but not Exception,
    # so if we catch BaseException, we must ignore GeneratorExit.
    # if we only catch Exception, we don't have to worry about that.
    #   except BaseException as e:
    #         if not isinstance(e, GeneratorExit):
    #             print("exception:", e)
    #             raise
    except Exception as e:
        print("exception:", e)
        raise
    print(stats.format())


def main(argv) -> int:
    if len(argv) != 2:
        print('usage: csv-reading <path>')
        return 1
    for row in csv_file_generator(argv[1]):
        print(row)
    return 0


if __name__ == '__main__':
    exit(main(sys.argv))
