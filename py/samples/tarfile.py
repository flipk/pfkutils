
import zipfile
import tarfile

def unziptardir(path: str, tarfile: str):
    cli = PbClient.PbClient('atlc04125')
    if not cli.ok:
        print('ERROR: pbclient init failed')
        return
    if not cli.login(USER, PASSWORD):
        print('ERROR: pbclient login failed')
        return
    success, reason, entries = cli.list_dir(path)
    if not success:
        print(f'ERROR: pbclient failed to list dir: {reason}')
        return
    tarfile.ENCODING = 'utf-8'
    t = tarfile.open(tarfile, mode='w:gz',
                     compresslevel=1,
                     encoding='utf-8')
    de: MyFS_pb2.DirEntry
    for de in entries:
        if de.type == MyFS_pb2.TYPE_FILE:
            fullpath = path + '/' + de.entry_name
            success, reason, zipfilebody = cli.request_file(fullpath)
            if not success:
                print(f'ERROR failed to retrieve {fullpath}: {reason}')
            else:
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
                            # strip out all the subdir names,
                            # and leave just the file.
                            slashpos = fn.rfind('/')
                            if slashpos > 0:
                                fn = fn[slashpos + 1:]
                            if fn[0] == '.':  # no leading dots!!
                                fn = fn[1:]
                            if fn[0] == '_':  # no leading underscores!!
                                fn = fn[1:]
                            print(f'     {len(filebody):8d} {fn}')
                            ti = tarfile.TarInfo(fn)
                            ti.size = len(filebody)
                            t.addfile(ti, io.BytesIO(filebody))
    t.close()
