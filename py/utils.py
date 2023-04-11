from typing import Callable
import os
import re
import collections.abc
import gc
import tracemalloc


def find_files(start: str, endswith: str) -> (list | None, str):
    todo = [start]
    try:
        while len(todo) > 0:
            p = todo[0]
            todo = todo[1:]
            de: os.DirEntry
            for de in os.scandir(p):
                e = p + '/' + de.name
                if de.is_dir():
                    todo.append(e)
                elif de.is_file():
                    if e.endswith(endswith):
                        yield p, de.name
    except Exception as e:
        print('scandir:', e)
        raise


def _rmtree(path: str, remove_top: bool, tprint: Callable):
    todo = [path]
    if remove_top:
        rmdirtodo = [path]
    else:
        rmdirtodo = []
    while len(todo) > 0:
        p = todo[0]
        todo = todo[1:]
        d: collections.abc.Iterable
        with os.scandir(p) as d:
            de: os.DirEntry
            for de in d:
                e = p + '/' + de.name
                if de.is_dir():
                    todo.append(e)
                    rmdirtodo.insert(0, e)
                else:
                    os.unlink(e)
    removedir: str
    try:
        for removedir in rmdirtodo:
            os.rmdir(removedir)
    except OSError as e:
        tprint(f'RMDIR {path} ERROR: {e}, trying again\n')
        pass


def rmtree(path: str, remove_top: bool = True, tprint: Callable = print):
    # sometimes we're in a race with a dying process and it creates
    # a file just after we scandir and just before we rmdir.
    # so we try/except around the rmdir, and repeat the loop
    # while the root still exists. we'll get it eventually.
    if remove_top:
        while os.path.exists(path):
            _rmtree(path, remove_top, tprint)
    else:
        if os.path.exists(path):
            _rmtree(path, remove_top, tprint)


def change_suffix(path: str, oldsuffixes: str, newsuffix: str) -> None | str:
    m = re.search("^(.*)\\.(" + oldsuffixes + ")$", path)
    if not m:
        return None
    return m.group(1) + "." + newsuffix


def init_trace():
    tracemalloc.start()


def make_trace(fn: str):
    gc.collect(0)
    gc.collect(1)
    gc.collect(2)
    snap = tracemalloc.take_snapshot()
    top_stats = snap.statistics('lineno')
    with open(fn, 'w') as fd:
        for s in top_stats:
            fd.write(f'{s}\n')


def dump(obj, name: str = 'arg', doprint: Callable = print):
    doprint(f'{name}: type({type(obj)})')
    if isinstance(obj, dict):
        for key in obj.keys():
            doprint(f'{name}[{key}]: {type(obj[key])}: {obj[key]}')
    for attr in dir(obj):
        a = getattr(obj, attr)
        doprint(f'{name}.{attr}: {type(a)}: {a}')
