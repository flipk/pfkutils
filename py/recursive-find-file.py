#!/usr/bin/env python3

import sys
import os
from typing import Union


def recursive_find_file_v1(root: str,
                           ret: list = []) -> (Union[list,
                                                     None],
                                               str):
    """
    this is the truly recursive version which passes the return list
    as an arg to itself to build the list.
    :param root: str: the root directory to perform the search
    :param ret: list: used for recursion; user should not populate this.
    returns: (filelist,"") or (None,errorstring)
    """
    try:
        with os.scandir(root) as d:
            de: os.DirEntry
            for de in d:
                fullpath = root + '/' + de.name
                if de.is_file():
                    ret.append(fullpath)
                elif de.is_dir():
                    subdir_files, errstring = \
                        recursive_find_file_v1(fullpath, ret)
                    # ignore return value.

    except Exception as e:
        print('scandir:', e)
        return None, e

    return ret, ""


def recursive_find_file_v2(root: str) -> (Union[list,
                                                None],
                                          str):
    """
    this version is an iterative generator, and does not recurse.
    :param root: str: the root directory to perform the search
    returns: a filename at a time.
    """
    todo = [root]
    try:
        while len(todo) > 0:
            p = todo[0]
            todo = todo[1:]
            with os.scandir(p) as d:
                de: os.DirEntry
                for de in d:
                    e = p + '/' + de.name
                    if de.is_dir():
                        todo.append(e)
                    elif de.is_file():
                        yield e

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
        print('scandir:', e)
        raise


def main1(argv: list) -> int:
    if len(argv) != 2:
        print('usage:  rff <rootdir>')
        return 1
    files, errstring = recursive_find_file_v1(argv[1])
    if not files:
        print('failed:', errstring)
        return 1
    for f in files:
        print(f)
    return 0


def main2(argv: list) -> int:
    if len(argv) != 2:
        print('usage:  rff <rootdir>')
        return 1
    for f in recursive_find_file_v2(argv[1]):
        print(f)
    return 0


if __name__ == '__main__':
    # exit(main1(sys.argv))
    exit(main2(sys.argv))
