#!/usr/bin/env python3

import subprocess
import sys
import os
import fcntl
import threading
from typing import IO, Any
import select


def nonblock_file(fd: IO[Any]):
    fileno = fd.fileno()
    flags = fcntl.fcntl(fileno, fcntl.F_GETFL)
    fcntl.fcntl(fileno, fcntl.F_SETFL, flags | os.O_NONBLOCK)


def reader(fd: IO[bytes]):
    while True:
        # fd is nonblocking, so the only way to block on it
        # is with select.
        # nonblock is the only way to get short reads (???)
        r, w, x = select.select([fd], [], [], .2)
        if len(r) > 0:
            buf = fd.read()
            if buf:
                print(f'got buf: {len(buf)}:{buf}')
            else:
                break
    print('reader done')


# noinspection PyUnusedLocal
def main(args: list[str]) -> int:
    # rd, wr = os.pipe()
    # flags = fcntl.fcntl(rd, fcntl.F_GETFL)
    # fcntl.fcntl(rd, fcntl.F_SETFL, flags | os.O_NONBLOCK)
    # rdfd = io.open(rd, 'rb')
    # wrfd = io.open(wr, 'wb')
    # wrfd.write(b'poo')
    # wrfd.flush()
    # buf = rdfd.read()
    # print(f'got buf of len {len(buf)}:{buf}')

    cmd = ['./subprocess-sample.sh']
    p = subprocess.Popen(cmd,
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    nonblock_file(p.stdout)
    rdr = threading.Thread(target=reader, args=[p.stdout])
    rdr.start()
    p.wait()
    rdr.join()
    return 0


if __name__ == '__main__':
    exit(main(sys.argv))
