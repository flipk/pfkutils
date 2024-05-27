#!/usr/bin/env python3

# this program scans the local hard drive for all files and directories
# which are on BTRFS filesystems (on fedora, "/" and "/home").  it scans
# each dir or file for mode, inode#, and size, and logs entries in a database.
# each time this program is run it looks for changes since the last run.
# this could be useful for incremental backups, but that's not what I wrote
# it for. fedora has a nasty bug in BTRFS where sometimes file modes get
# changed, and I think it happens when a foreign BTRFS filesystem is mounted.
# this tool will detect random changes and call them out.

# see https://docs.python.org/3/library/sqlite3.html
# see https://www.sqlite.org/lang.html
import os
import time
import stat
import sqlite3
import queue
import threading
from enum import Enum
from threading import Thread
from typing import Union, Callable, TextIO


class MyFileType(Enum):
    """i only care about dirs and files, not symlinks, pipes, devs, etc"""
    DIR = 1
    FILE = 2


class MyFile:
    """
    this class describes a single file or directory. it also maps directly
    to a database entry row in the database, though look in FsInfoDb class
    for more info about actually getting and putting this object.
    """
    ok: bool
    type: MyFileType  # only exists if ok=True
    fname: str  # only exists of ok=True
    inode: int  # only exists of ok=True
    devid: int  # only exists of ok=True
    mode: int  # only exists of ok=True
    size: int  # only exists of ok=True
    stamp: int
    err: Union[Exception, str, None]  # only exists if ok=False

    def __init__(self, fname: str, do_stat: bool):
        """
        initiale a MyFile object, optionally by doing an lstat() call and
        filling out the relevant information. if do_stat is true, perform
        lstat() and then set(); if do_stat is false, don't fill anything out
        (the latter is useful during a database fetch).
        """
        self.fname = fname
        if do_stat:
            try:
                sb = os.lstat(self.fname)
                self.set(sb.st_ino, sb.st_dev, sb.st_mode, sb.st_size)
            except Exception as e:
                self.err = e
                self.ok = False
        else:
            self.ok = False
            self.err = None
        self.stamp = 0

    def set(self, inode: int, devid: int, mode: int, size: int):
        """
        set information fields about this file in one single command.
        """
        if stat.S_ISDIR(mode):
            self.type = MyFileType.DIR
        elif stat.S_ISREG(mode):
            self.type = MyFileType.FILE
        else:
            self.ok = False
            self.err = f'is not dir or type'
            return
        self.inode = inode
        self.devid = devid
        self.mode = mode
        self.size = size
        self.ok = True
        self.err = None

    def __eq__(self, other) -> bool:
        """
        overload the "==" operator so we can just say "f == g" to see
        if a file's vitals have changed.
        """
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
        if self.devid != other.devid:
            return False
        if self.size != other.size:
            return False
        return True

    def __str__(self):
        """
        human readable form of this object, which appears in
        the fs-changes.txt file
        """
        if self.ok:
            typestr = "F" if self.type == MyFileType.FILE else "D"
            return (f'{typestr} o{self.mode:06o} '
                    f'{self.devid:5d}:{self.inode:09d} '
                    f'{self.size:11d} {self.fname}')
        else:
            return f'fname:{self.fname}: {self.err}'


class FsInfoDb:
    """
    this class encapsulates file entries in an sqlite3 database. it contains
    all the schema init commands as well as getters and putters.
    """
    _conn: sqlite3.Connection
    _SCHEMA = [
        'create table files (fname string, inode int,'
        ' devid int, mode int, size int, stamp int)',
        'create unique index files_fname on files (fname)',
        'create index files_inode on files (inode, devid)',
    ]

    def __init__(self, db_file: str):
        """
        open the database; create the database and create the tables and
        indexes if they don't exist.
        """
        newfile = True
        if os.access(db_file, os.W_OK):
            newfile = False
        if newfile:
            try:
                os.unlink(db_file)
            except FileNotFoundError:
                pass
        try:
            self._conn = sqlite3.connect(db_file)
        except sqlite3.OperationalError as e:
            print(f'opening: {db_file}: {e}')
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
        """commit any outstanding transactions to the database. required
        since we use a transaction journal."""
        self._conn.commit()

    def insert(self, f: MyFile):
        """add a record for a file to the database; assumes it
        doesn't already exist."""
        self._conn.execute('insert or replace into files '
                           '(fname, inode, devid, mode, size, stamp) values '
                           '(?,?,?,?,?,?)',
                           (f.fname, f.inode,
                            f.devid, f.mode, f.size, f.stamp))

    def update(self, f: MyFile):
        """update an existing record; matches by filename."""
        self._conn.execute('update files '
                           'set (inode, devid, mode, size, stamp) = '
                           '(?,?,?,?,?) where fname = ?',
                           (f.inode, f.devid,
                            f.mode, f.size, f.stamp, f.fname))

    def find(self, fname: str) -> Union[MyFile, None]:
        """search for an entry by filename; returns None if not found."""
        cur = self._conn.cursor()
        for res in cur.execute('select fname, inode, devid, mode, size, stamp '
                               'from files '
                               'where fname = ?',
                               (fname,)):
            f = MyFile(fname, False)
            f.set(res[1], res[2], res[3], res[4])
            f.stamp = res[5]
            return f
        return None

    def find_old(self, stamp: int):
        """search for all entries in the database not using the current
        timestamp. this is an iterator."""
        cur = self._conn.cursor()
        for res in cur.execute('select fname, inode, devid, mode, size, stamp '
                               'from files '
                               'where stamp != ?',
                               (stamp,)):
            f = MyFile(res[0], False)
            f.set(res[1], res[2], res[3], res[4])
            f.stamp = res[5]
            yield f


class Walker:
    """
    this class knows how to recursively scandir but stay within a single
    filesystem, or a set of them. calls a callback for each item found.
    """
    _todo: queue.Queue[MyFile]
    _devids: list[int]
    new_stamp: int

    def __init__(self, root_dirs: list[str]):
        """initialize the work queue with a set of root dirs."""
        self._devids = []
        self._todo = queue.Queue()
        self.new_stamp = int(time.time())
        f = MyFile('/', True)
        self._todo.put(f)
        for p in root_dirs:
            f = MyFile(p, True)
            self._devids.append(f.devid)

    def walk(self, func: Union[Callable[[MyFile, int], None], None]):
        """recursively iterate over all directories and files, calling
        the supplied worker function for each entry."""
        while not self._todo.empty():
            f1 = self._todo.get()
            func(f1, self.new_stamp)
            for de in os.scandir(f1.fname):
                f2 = MyFile(de.path, True)
                if f2.ok and f2.devid in self._devids:
                    if f2.type == MyFileType.DIR:
                        self._todo.put(f2)
                    else:
                        func(f2, self.new_stamp)


class BtrfsChecker:
    _db: FsInfoDb
    _w: Walker
    _uid: int
    _chglog: TextIO
    # _dbglog: TextIO
    _files: int
    _inserts: int
    _changes: int
    _deletions: int
    _stopq: queue.Queue
    _thread: Thread

    def __init__(self, uid: int, root_dirs: list[str],
                 db_file: str, change_log_file: str):
        self._uid = uid
        # give up root perms while opening the files.
        os.seteuid(uid)
        self._db = FsInfoDb(db_file)
        self._w = Walker(root_dirs)
        self._chglog = open(change_log_file, 'w')
        # self._dbglog = open('debug.txt', 'w')
        self._files = 0
        self._inserts = 0
        self._changes = 0
        self._deletions = 0
        self._stopq = queue.Queue()
        self._thread = threading.Thread(target=self._printer_thread)

    def __del__(self):
        self._chglog.close()
        # self._dbglog.close()
        del self._db
        del self._w

    def _printer_thread(self):
        go = True
        while go:
            try:
                m = self._stopq.get(timeout=0.25)
                if m:
                    go = False
            except queue.Empty:
                pass
            print(f'\r{self._files} files, {self._inserts} inserts, '
                  f'{self._changes} changes {self._deletions} deletions',
                  end='')
        print('')

    def _handle_file(self, f: MyFile, new_stamp: int):
        # self._dbglog.write(f'{f}\n')
        self._files += 1
        g = self._db.find(f.fname)
        f.stamp = new_stamp
        if not g:
            self._chglog.write(f'I {f}\n')
            self._inserts += 1
            self._db.insert(f)
            return
        if f != g:
            self._chglog.write(f'C {f}\n')
            self._changes += 1
        f.stamp = new_stamp
        self._db.update(f)

    def walk(self) -> int:
        self._thread.start()
        # switch to root to walk the file system.
        os.seteuid(0)
        self._w.walk(self._handle_file)
        # give up root
        os.seteuid(self._uid)
        self._db.commit()
        for f in self._db.find_old(self._w.new_stamp):
            self._chglog.write(f'D {f}\n')
            self._deletions += 1
        self._stopq.put(1)
        self._thread.join()
        return self._changes


def main():
    db_file = 'fsinfo.db'
    change_log_file = 'fs-changes.txt'
    if os.geteuid() != 0:
        print('please run as root, and set USERID to the user id of the real user')
        return 1
    uid = os.environ.get('USERID')
    if uid:
        uid = int(uid)
    else:
        print('please set USERID to the numeric user id of the real user')
        return 1
    bc = BtrfsChecker(uid, ['/', '/home'], db_file, change_log_file)
    changes = bc.walk()
    print(f'detected {changes} changes, see {change_log_file}')
    return 0


if __name__ == '__main__':
    exit(main())
