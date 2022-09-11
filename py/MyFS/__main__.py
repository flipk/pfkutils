#!/usr/bin/env python3

import os
import sys
from MyFS import PbServer
from MyFS import PbClient
from MyFS import MyFS_pb2

username = 'user'
password = 'password'

def main_server(root_dir: str):
    s = PbServer.PbServer(root_dir, username, password)
    if s.ok:
        try:
            s.serve()
        except KeyboardInterrupt:
            print("\nI: exiting due to keyboard interrupt.")
    else:
        print("E: server failed to start")


def main_client_cmdline() -> bool:
    c = PbClient.PbClient(sys.argv[3])
    if not c.ok:
        print('pbclient init failed')
        return False

    if not c.login(username, password):
        print('login to file server failed! exiting..')
        return False

    if sys.argv[2] == 'ls':
        success, reason, entries = c.list_dir(sys.argv[4])
        if success:
            e: MyFS_pb2.DirEntry
            for e in entries:
                if e.type == MyFS_pb2.TYPE_DIR:
                    print('D          ', e.entry_name)
                elif e.type == MyFS_pb2.TYPE_FILE:
                    print('F', f'{e.size:9d}', e.entry_name)
            return True
        else:
            print(reason)

    elif sys.argv[2] == 'get':
        fn = sys.argv[4]
        success, reason, filebody = c.request_file(fn)
        if success:
            bn = os.path.basename(fn)
            print('retrieved file', bn, 'of size', len(filebody))
            try:
                outf = open(bn, 'wb')
                outf.write(filebody)
                outf.close()
                return True
            except Exception as e:
                print('failed to write output file:', e)
        else:
            print(reason)

    return False


def main() -> int:
    ok = False
    if len(sys.argv) >= 3:
        if sys.argv[1] == '-server':
            main_server(sys.argv[2])
            ok = True
        elif sys.argv[1] == '-client':
            if len(sys.argv) == 5:
                ok = main_client_cmdline()

    if not ok:
        print('usage:')
        print('   main.py -server : start proto-fs server')
        print('   main.py -test : proto-fs test client')
        print('   main.py -client ls <hostname> <dir> : list a directory on server')
        print('   main.py -client get <hostname> <file> : fetch a file from server')
        return 1
    return 0


if __name__ == '__main__':
    # fix utf-8 encoding errors on stdout.
    sys.stdout = open(sys.stdout.fileno(), mode='w',
                      encoding='utf8', buffering=1)
    exit(main())
