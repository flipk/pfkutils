#!/usr/bin/env python3

# see https://docs.python.org/3/library/sqlite3.html
# see https://www.sqlite.org/lang.html
import os
import stat
import sqlite3
from enum import Enum
from typing import Union, Callable


class FileType(Enum):
    DIR = 1
    FILE = 2


class FsInfoDb:
    _conn: sqlite3.Connection
    _DB_FILE = 'fsinfo.db'
    _SCHEMA = [
        'create table files (fname string, inode int, fsid int, mode int, size int)',
        'create unique index files_fname on files (fname)',
        'create index files_inode on files (inode, fsid)',
    ]

    def __init__(self):
        newfile = True
        if os.access(self._DB_FILE, os.W_OK):
            newfile = False
        if newfile:
            try:
                os.unlink(self._DB_FILE)
            except FileNotFoundError:
                pass
        try:
            self._conn = sqlite3.connect(self._DB_FILE)
        except sqlite3.OperationalError as e:
            print(f'opening: {self._DB_FILE}: {e}')
            exit(1)
        if newfile:
            for s in self._SCHEMA:
                try:
                    self._conn.execute(s)
                except sqlite3.OperationalError as e:
                    print(f'"{s}":')
                    print(f'error: {e}')
                    exit(1)

    def __del__(self):
        self._conn.commit()
        self._conn.close()

    def commit(self):
        self._conn.commit()

    class File:
        ok: bool
        type: FileType  # only exists if ok=True
        fname: str  # only exists of ok=True
        inode: int  # only exists of ok=True
        fsid: int  # only exists of ok=True
        mode: int  # only exists of ok=True
        size: int  # only exists of ok=True
        err: Union[Exception, str, None]  # only exists if ok=False

        def __init__(self, fname: str):
            self.fname = fname
            self.ok = False
            self.err = None

        def set(self, inode: int, fsid: int, mode: int, size: int):
            if stat.S_ISDIR(mode):
                self.type = FileType.DIR
            elif stat.S_ISREG(mode):
                self.type = FileType.FILE
            else:
                self.ok = False
                self.err = f'is not dir or type'
                return
            self.inode = inode
            self.fsid = fsid
            self.mode = mode
            self.size = size
            self.ok = True
            self.err = None

        def stat(self) -> bool:
            try:
                sb = os.stat(self.fname)
                self.set(sb.st_ino, sb.st_dev, sb.st_mode, sb.st_size)
            except Exception as e:
                self.err = e
                self.ok = False
            return self.ok

        def __eq__(self, other):
            if self.fname != other.fname:
                return False
            if not self.ok or not other.ok:
                return False
            if self.type != other.type:
                return False
            if self.mode != other.mode:
                return False
            if self.inode != other.inode:
                return False
            if self.fsid != other.fsid:
                return False
            if self.size != other.size:
                return False
            return True

        def __str__(self):
            if self.ok:
                return (f'{self.fname}: {self.type} o{self.mode:o} '
                        f'sz {self.size} {self.fsid}:{self.inode}')
            else:
                return f'fname:{self.fname}, err:{self.err}'

    def insert(self, f: File):
        self._conn.execute('insert or replace into files '
                           '(fname, inode, fsid, mode, size) values '
                           '(?,?,?,?,?)',
                           (f.fname, f.inode,
                            f.fsid, f.mode, f.size))

    def update(self, f: File):
        self._conn.execute('update files '
                           'set (inode, fsid, mode, size) = '
                           '(?,?,?,?) where fname = ?',
                           (f.inode, f.fsid,
                            f.mode, f.size, f.fname))

    def find(self, fname: str) -> Union[File, None]:
        cur = self._conn.cursor()
        for res in cur.execute('select fname, inode, fsid, mode, size '
                               'from files '
                               'where fname = ?',
                               (fname,)):
            f = FsInfoDb.File(fname)
            f.set(res[1], res[2], res[3], res[4])
            return f
        return None


class Walker:
    todo: list[FsInfoDb.File]
    fsids: list[int]

    def __init__(self):
        self.fsids = []
        self.todo = []
        for p in ['/', '/home']:
            f = FsInfoDb.File(p)
            f.stat()
            self.fsids.append(f.fsid)
            self.todo.append(f)

    def walk(self, func: Union[Callable[[FsInfoDb.File], None], None]):
        for f1 in self.todo:
            if func:
                func(f1)
            else:
                print(f'd: {f1}')
            for de in os.scandir(f1.fname):
                f2 = FsInfoDb.File(de.path)
                f2.stat()
                if f2.ok and f2.fsid in self.fsids:
                    if f2.type == FileType.DIR:
                        self.todo.append(f2)
                    else:
                        if func:
                            func(f2)
                        else:
                            print(f'f: {f2}')


class BtrfsChecker:
    db: FsInfoDb
    w: Walker

    def __init__(self):
        self.db = FsInfoDb()
        self.w = Walker()

    def _handle_file(self, f: FsInfoDb.File):
        g = self.db.find(f.fname)
        if not g:
            self.db.insert(f)
            return
        if f == g:
            return
        print(f'old: {g}\nnew: {f}')
        self.db.update(f)

    def walk(self):
        self.w.walk(self._handle_file)
        # self.w.walk(None)
        self.db.commit()


def main():
    bc = BtrfsChecker()
    bc.walk()
    return 0


if __name__ == '__main__':
    exit(main())
