#!/usr/bin/env python3

import os
import sys
from MyFS_Config import MyFSConfig
from MyFS_Client import MyFSClient
from MyFS_Server import MyFSTCPServer
import MyFS_pb2


def main_server(cfg: MyFSConfig):
    s = MyFSTCPServer(cfg)
    if s.ok:
        try:
            s.serve()
        except KeyboardInterrupt:
            print("\nI: exiting due to keyboard interrupt.")
    else:
        print("E: server failed to start")


def main_client_cmdline(cfg: MyFSConfig, args: list[str]) -> bool:
    c = MyFSClient(cfg)
    if not c.ok:
        print('pbclient init failed')
        return False

    if not c.login():
        print('login to file server failed! exiting..')
        return False

    if args[0] == 'ls':
        success, reason, entries = c.list_dir(args[1])
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

    elif args[0] == 'lsarch':
        success, reason, entries = c.list_archive(args[1])
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

    elif args[0] == 'check':
        fn = args[1]
        success, reason, size = c.get_file(fn, just_check=True)
        if success:
            print(f'file exists on server of size {size}')
            return True
        else:
            print(reason)

    elif args[0] == 'get':
        fn = args[1]
        success, reason, filebody = c.get_file(fn)
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

    elif args[0] == 'put':
        fn = args[1]
        try:
            inf = open(fn, 'rb')
            filebody = inf.read()
            inf.close()
        except Exception as e:
            print(f'failed to read input file: {e}')
            return False
        success, reason = c.put_file(fn, filebody)
        if success:
            bn = os.path.basename(fn)
            print('sent file', bn, 'of size', len(filebody))
            return True
        else:
            print(reason)

    elif args[0] == 'delete':
        fn = args[1]
        success, reason = c.delete_file(fn)
        if success:
            print(f'file {fn} is deleted')
            return True
        else:
            print(reason)

    elif args[0] == 'mkdir':
        dn = args[1]
        success, reason = c.make_dir(dn)
        if success:
            print(f'dir {dn} is created')
            return True
        else:
            print(reason)

    elif args[0] == 'rmdir':
        dn = args[1]
        success, reason = c.remove_dir(dn)
        if success:
            print(f'dir {dn} is deleted')
            return True
        else:
            print(reason)

    elif args[0] == 'getarch':
        fn = args[1]
        success, reason, filebodies = c.get_archive(fn, args[2:])
        if success:
            for ind in range(len(filebodies)):
                exfn = args[2+ind]
                print(f'zip {fn} file {exfn} is len {len(filebodies[ind])}')
                with open(exfn, 'wb') as fd:
                    fd.write(filebodies[ind])
            return True
        else:
            print(reason)

    elif args[0] == 'getarchsvr':
        fn = args[1]
        members = []
        dests = []
        for ind in range(2, len(args), 2):
            members.append(args[ind])
            dests.append(args[ind+1])
        success, reason, filebodies = c.get_archive(fn, members, dests)
        if success:
            print(f'successful extraction')
        else:
            print(reason)

    elif args[0] == 'rmtree':
        dn = args[1]
        success, reason = c.remove_tree(dn)
        if success:
            print(f'directory tree {dn} is removed')
            return True
        else:
            print(reason)

    else:
        print(f'ERROR unknown command {args[0]}')

    return False


def usage():
    print('usage:')
    print('   main.py -server : start proto-fs server')
    print('   main.py ls <dir> : list a directory on server')
    print('   main.py lsarch <archive_file> : list contents of a zip or tar archive')
    print('   main.py get <file> : fetch a file from server')
    print('   main.py check <file> : check if a file exists')
    print('   main.py getarch <archive_file> <file>..<file> : get contents of archive')
    print('   main.py getarchsvr <archive_file> <file> <dest>..<file> <dest> : extract archive, save on server')
    print('   main.py put <file> : send a file to server')
    print('   main.py delete <file> : delete a file')
    print('   main.py mkdir <dir> : make a directory')
    print('   main.py rmdir <dir> : remove a directory')
    print('   main.py rmtree <dir> : recursively remove a tree of files (DANGER)')


def main() -> int:
    cfg = xxxxxxxxxxxxxxxxxxxx
    ok = False
    if len(sys.argv) >= 2:
        if sys.argv[1] == '-server':
            main_server(cfg)
            ok = True
        elif len(sys.argv) >= 3:
            main_client_cmdline(cfg, sys.argv[1:])
            ok = True
    if not ok:
        usage()
    return 0


if __name__ == '__main__':
    exit(main())
