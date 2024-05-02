#!/usr/bin/env python3

import sys
import time
import threading
from mysubproc import MyPipe


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
    p = MyPipe(cmd)

    def reader(pp: MyPipe):
        out = pp.reader()
        print(f'got output len:{len(out)}')

    t = threading.Thread(target=reader, args=[p])
    t.start()

    p.stdin.write('input 1\n')
    print('wrote input 1')
    p.stdin.flush()
    time.sleep(2)
    p.stdin.write('input 2\n')
    print('wrote input 2')
    p.stdin.flush()
    time.sleep(2)
    p.stdin.write('input 3\n')
    print('wrote input 3')
    p.stdin.flush()
    time.sleep(2)
    p.stdin.write('input 4\n')
    print('wrote input 4')
    p.stdin.flush()
    time.sleep(2)
    p.stdin.write('input 5\n')
    print('wrote input 5')
    p.stdin.flush()
    time.sleep(2)

    t.join()
    return 0


if __name__ == '__main__':
    exit(main(sys.argv))
