#!/usr/bin/env python3

import os
import re
from re import Pattern


class OpenFiles:

    class PidFiles:
        pid: int
        cmdline: list[str]
        filenames: list[str]

        def __init__(self, pid: int):
            self.pid = pid
            try:
                with open(f'/proc/{pid}/cmdline', 'rb') as fd:
                    buf = fd.read().strip(b'\r\n\x00')
                    flds = buf.split(b'\x00')
                    self.cmdline = [f.decode()
                                    for f in flds]
            except Exception as e:
                self.cmdline = [f'{e}']
            self.filenames = []

        def readlink(self, link: str):
            try:
                afile = os.readlink(link)
                if afile == '/':
                    return None
                self.filenames.append(afile)
            except FileNotFoundError:
                pass

    pidfiles: dict[int, PidFiles]
    _numpattern: Pattern[str]

    def __init__(self):
        self.pidfiles = {}
        self._numpattern = re.compile('^[0-9]+$')

    def rescan(self):
        self.pidfiles.clear()
        for de in os.scandir('/proc'):
            if self._numpattern.search(de.name):
                pid = int(de.name)
                pf = self.PidFiles(pid)
                self.pidfiles[pid] = pf
                piddir = '/proc/' + de.name
                pf.readlink(piddir + '/cwd')
                pf.readlink(piddir + '/exe')
                pf.readlink(piddir + '/root')
                fddir = piddir + '/fd'
                try:
                    for fdde in os.scandir(fddir):
                        pf.readlink(fddir + '/' + fdde.name)
                    mfdir = piddir + '/map_files'
                    for mfde in os.scandir(mfdir):
                        pf.readlink(mfdir + '/' + mfde.name)
                except FileNotFoundError:
                    pass

    def filter(self, startswith: str):
        for pf in self.pidfiles.values():
            for of in pf.filenames:
                if of.startswith(startswith):
                    yield pf.pid, pf.cmdline, of

    def __str__(self):
        ret = ''
        for pf in self.pidfiles.values():
            for of in pf.filenames:
                cmdline = ' '.join(pf.cmdline)
                ret += f'{pf.pid}({cmdline}): {of}\n'
        return ret


def main():
    of = OpenFiles()
    of.rescan()
    for pid, cmdline, fn in of.filter('/proc'):
        cmdline = ' '.join(cmdline)
        print(f'found: {pid}({cmdline}):{fn}')
    pass


if __name__ == '__main__':
    exit(main())
