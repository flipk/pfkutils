#!/usr/bin/env python3

import os
import sys
import argparse
import socket
import select
import threading
import time

class Ipipe:
    def __init__(self):
        parser = argparse.ArgumentParser(prog='ipipe4.py',
                                         description='internet pipe fitting')
        parser.add_argument('-v', dest='verbose', action='store_true',
                            help='verbose status prints')
        parser.add_argument('-n', dest='noinput', action='store_true',
                            help='do not read from input')
        parser.add_argument('-O', dest='discard', action='store_true',
                            help='discard output')
        parser.add_argument('-i', dest='infile', nargs=1, type=str,
                            help='input from filename')
        parser.add_argument('-o', dest='outfile', nargs=1, type=str,
                            help='output to filename')
        parser.add_argument('ipport', nargs='+', type=str,
                            help='[port | ipaddr port ]', )
        self.args = parser.parse_args()
        self.verbose = self.args.verbose
        self.listener = False
        self.sock: socket.socket | None = None
        self.peer_addr = None
        if len(self.args.ipport) == 1:
            self.ipaddr = None
            self.port = int(self.args.ipport[0])
            self.listener = True
        elif len(self.args.ipport) == 2:
            self.ipaddr = self.args.ipport[0]
            self.port = int(self.args.ipport[1])
        else:
            parser.print_usage()
            exit(1)
        if self.args.discard:
            self.output = None
        elif self.args.outfile:
            self.output = open(self.args.outfile[0], 'wb')
        else:
            self.output = sys.stdout.buffer
        if self.args.noinput:
            self.input = None
        elif self.args.infile:
            self.input = open(self.args.infile[0], 'rb')
        else:
            self.input = sys.stdin.buffer
        self.bufsize = 64 * 1024
        self.byte_count = 0
        self.done = False

    @staticmethod
    def usage():
        print(f'usage:'
              f'   i4 [-v] [-o file] port         :  listen'
              f'   i4 [-v] [-o file] IPADDR PORT  : connect')

    def get_connection(self) -> bool:
        try:
            s = socket.socket()
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
            s.bind(('', self.port))
            s.listen(1)
            self.sock, self.peer_addr = s.accept()
            s.close()
            if self.verbose:
                print(f'new connection from {self.peer_addr[0]} {self.peer_addr[1]}')
        except Exception as e:
            print(f'error during socket/accept/listen: {e}')
            return False
        return True

    def do_connect(self) -> bool:
        self.peer_addr = (self.ipaddr, self.port)
        try:
            s = socket.socket()
            s.connect(self.peer_addr)
            self.sock = s
            if self.verbose:
                print(f'new connection to {self.peer_addr[0]} {self.peer_addr[1]}')
        except Exception as e:
            print(f'error during socket/connect: {e}')
            return False
        return True

    def forward_loop(self):
        rlist = [self.sock.fileno()]
        if self.input:
            rlist.append(self.input.fileno())
            os.set_blocking(self.input.fileno(), False)
        self.done = False
        while not self.done:
            res = select.select(rlist, [], [], 1.0)
            for fd in res[0]:
                if fd == self.sock.fileno():
                    buf = self.sock.recv(self.bufsize)
                    if len(buf) == 0:
                        self.done = True
                        break
                    self.output.write(buf)
                    self.byte_count += len(buf)
                elif self.input and fd == self.input.fileno():
                    buf = self.input.read(self.bufsize)
                    if len(buf) == 0:
                        self.done = True
                        break
                    self.sock.send(buf)
                    self.byte_count += len(buf)
        self.output.flush()
        return

    @staticmethod
    def human_readable(value):
        if value < 1000:
            return f'{value}'
        if value < 1000000:
            value /= 1000
            return f'{value:.3f} K'
        if value < 1000000000:
            value /= 1000000
            return f'{value:.3f} M'
        value /= 1000000000
        return f'{value:.3f} G'

    def stats(self):
        time_start = time.time()
        while not self.done:
            time.sleep(0.25)
            time_now = time.time()
            time_diff = time_now - time_start
            if self.byte_count > 0:
                rate = self.byte_count / time_diff
            else:
                rate = 0
            bitrate = self.human_readable(rate * 8)
            rate = self.human_readable(rate)

            bc = self.human_readable(self.byte_count)
            print(f'  {bc}B in {time_diff:.1f}s '
                  f'({rate}Bps {bitrate}bps)       \r', end='')
        print('')

    def main(self) -> int:
        if self.verbose:
            if self.listener:
                print(f'listening on port {self.port}')
            else:
                print(f'connecting to ip {self.ipaddr} port {self.port}')
            if self.args.infile:
                print(f'infile = {self.args.infile[0]}')
            if self.args.outfile:
                print(f'outfile = {self.args.outfile[0]}')
            if not self.input:
                print('ignoring input')
            if not self.output:
                print('discarding output')
        if self.listener:
            if not self.get_connection():
                return 1
        else:
            if not self.do_connect():
                return 1
        stats_thread = None
        if self.verbose:
            stats_thread = threading.Thread(target=self.stats, daemon=True)
            stats_thread.start()
        self.forward_loop()
        if stats_thread:
            stats_thread.join()
        return 0


if __name__ == '__main__':
    app = Ipipe()
    try:
        exit(app.main())
    except KeyboardInterrupt:
        print('\nINTERRUPT')
        exit(1)
