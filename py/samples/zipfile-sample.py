
import zipfile
import subprocess
import shutil
import os

# if lowriter takes more than 30 seconds, i dont think we want
# to keep the output anyway. harumph.
LOWRITER_CONVERSION_TIMEOUT = 30

def main_test_client():
    c = PbClient.PbClient('atlc04125')
    if not c.ok:
        print('pbclient init failed')
        return
    if not c.login(USER, PASSWORD):
        print('login to file server failed! exiting..')
        return
    path = 'Information/WI_Sheet'
    success, reason, entries = c.list_dir(path)
    if not success:
        print('failed to list dir:', reason)
        return
    de: MyFS_pb2.DirEntry
    try:
        shutil.rmtree('output', ignore_errors=True)
        os.mkdir('output')
    except Exception as e:
        print('cannot make output directory:', e)
        return
    for de in entries:
        if de.type == MyFS_pb2.TYPE_DIR:
            print('  dir', de.entry_name)
        elif de.type == MyFS_pb2.TYPE_FILE:
            fullpath = path + '/' + de.entry_name
            success, reason, zipfilebody = c.request_file(fullpath)
            if not success:
                print('failed to retrieve', fullpath, ':', reason)
                break
            print(f'file {de.size:8d} {de.entry_name}')
            if fullpath.endswith('.zip') or fullpath.endswith('.ZIP'):
                with zipfile.ZipFile(io.BytesIO(zipfilebody)) as z:
                    for fn in z.namelist():
                        filebody = z.read(fn)
                        # some of the zip files have subdirectory trees.
                        # the subdirs themselves come across as zero length
                        # entries, skip them.
                        if len(filebody) == 0:
                            continue
                        print(f'     {len(filebody):8d} {fn}')
                        # strip out all the subdir names, and leave just the file.
                        slashpos = fn.rfind('/')
                        if slashpos > 0:
                            fn = fn[slashpos + 1:]
                        if fn[0] == '.':   # no leading dots!!
                            fn = fn[1:]
                        if fn[0] == '_':   # no leading underscores!!
                            fn = fn[1:]
                        fn = 'output/' + fn  # put it in the output dir now.
                        if fn.endswith('.docx') or fn.endswith('.DOCX'):
                            # needs an encode because some filenames have utf8 in them.
                            with open(fn.encode('utf-8'), 'wb') as writefd:
                                writefd.write(filebody)
                        elif fn.endswith('.doc') or fn.endswith('.DOC'):
                            # needs an encode because some filenames have utf8 in them.
                            # write the file so lowriter has something to read.
                            with open(fn.encode('utf-8'), 'wb') as writefd:
                                writefd.write(filebody)
                            try:
                                # libreoffice gets STUCK on some files!!! so we need timeout=
                                # to guard against that.
                                cp = subprocess.run(['/usr/lib/libreoffice/program/soffice.bin',
                                                     '--writer', '--convert-to', 'docx',
                                                     '--outdir', 'output', fn.encode('utf-8')],
                                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                                    timeout=LOWRITER_CONVERSION_TIMEOUT)
                            except subprocess.TimeoutExpired as te:
                                cp = None
                                print(f'ERROR TIMED OUT CONVERTING {fn}:', te)
                            if cp:
                                # save the stdout and stderr of lowriter to it's own file
                                # so you can go look at it if you really need to, but it
                                # doesn't fudge up the regular output while running.
                                logfn = fn[0:len(fn) - 4] + '--conversion_log.txt'
                                with open(logfn.encode('utf-8'), 'wb') as logfd:
                                    logfd.write(cp.stdout)
                                # check that the resulting output file was actually created.
                                newfn = fn[0:len(fn) - 4] + '.docx'
                                if os.path.exists(newfn):
                                    statbuf = os.stat(newfn)
                                    print(f'   C {statbuf.st_size:8d} {newfn}')
                                else:
                                    print(f'ERROR did conversion fail? {newfn}')
            else:
                print('  not a zipfile, skipping')


def main() -> int:
    ok = False
    if len(sys.argv) >= 2:
        if sys.argv[1] == '-server':
            main_server()
            ok = True
        elif sys.argv[1] == '-client':
            if len(sys.argv) == 4:
                ok = main_client_cmdline()
        elif sys.argv[1] == '-test':
            main_test_client()
            ok = True

    if not ok:
        print('usage:')
        print('   main.py -server : start MyFS server')
        print('   main.py -test : MyFS test client')
        print('   main.py -client ls <dir> : list a directory on server')
        print('   main.py -client get <file> : fetch a file from server')
        return 1
    return 0


if __name__ == '__main__':
    # fix utf-8 encoding errors on stdout.
    sys.stdout = open(sys.stdout.fileno(), mode='w',
                      encoding='utf8', buffering=1)
    exit(main())
