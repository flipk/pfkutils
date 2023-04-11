import collections.abc
from typing import Dict, Callable, BinaryIO
import os
import socket
import zipfile
import tarfile
from PbConnection import PbConnection
from MyFS_Config import MyFSConfig
from TcpServer import TcpServer
import MyFS_pb2


class MyFSServer(PbConnection):
    _cfg: MyFSConfig
    # the key type for _handlers is actually MyFS_pb2.MessageType enum
    _handlers: Dict[int, Callable[[MyFS_pb2.Client2Server], bool]]
    _authenticated: bool
    _put_in_progress: bool
    _put_remaining: int
    _put_fd: BinaryIO | None

    def __init__(self, cfg: MyFSConfig, sock: socket.socket, addr):
        self._cfg = cfg
        self._ok = False
        self._authenticated = False
        self._put_in_progress = False
        self._put_remaining = 0
        self._put_fd = None
        self._handlers = {
            MyFS_pb2.LOGIN: self._handle_login,
            MyFS_pb2.GET_FILE: self._handle_get_file,
            MyFS_pb2.LIST_DIR: self._handle_list_dir,
            MyFS_pb2.PUT_FILE: self._handle_put_file,
            MyFS_pb2.FILE_CONTENTS: self._handle_file_contents,
            MyFS_pb2.DELETE: self._handle_delete,
            MyFS_pb2.MKDIR: self._handle_mkdir,
            MyFS_pb2.RMDIR: self._handle_rmdir,
            MyFS_pb2.LIST_ARCHIVE: self._handle_list_archive,
            MyFS_pb2.GET_ARCHIVE: self._handle_get_archive,
            MyFS_pb2.REMOVE_TREE: self._handle_remove_tree
        }
        super().__init__(sock, sock, addr, MyFS_pb2.Client2Server(),
                         self._callback_msg, None,
                         self._cfg.debug_text,
                         self._cfg.debug_binary)

    def _callback_msg(self, msg: MyFS_pb2.Client2Server):
        handler = self._handlers.get(msg.type, None)
        if handler:
            # LOGIN is the only message type allowed
            # if not yet authenticated.
            if self._authenticated or \
                    msg.type == MyFS_pb2.MessageType.LOGIN:
                ret = handler(msg)
                if not ret:
                    print('message handler said to kill it')
            else:
                print('unauthorized message attempted', msg)
        else:
            print('unhandled message:', msg)

    @staticmethod
    def _valid_path(path: str):
        if path[0] == '/':
            # absolute paths not permitted
            return False
        for c in path.split('/'):
            if c == '..':
                # escaping from root dir not permitted
                return False
        return True

    def _handle_login(self, c2s: MyFS_pb2.Client2Server) -> bool:
        print(f'got LOGIN message for user {c2s.login.user} from {self._addr}')
        ret = False
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.LOGIN_RESP
        if c2s.login.user == self._cfg.username and \
           c2s.login.password == self._cfg.password:
            self._authenticated = True
            print('user authenticated!')
            s2c.loginresp.success = True
            s2c.loginresp.reason = 'successful login, welcome'
            ret = True
        else:
            print(f'authentication failed, kicking off {self._addr}')
            s2c.loginresp.success = False
            s2c.loginresp.reason = 'invalid username or password'
        self.send_message(s2c)
        return ret

    def _handle_list_dir(self,
                         c2s: MyFS_pb2.Client2Server) -> bool:
        print(f'got LIST_DIR message from {self._addr} for dir {c2s.listdir.dirname}')
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.LIST_DIR_RESP
        s2c.listdirresp.success = False
        entry_count = 0
        if not self._valid_path(c2s.listdir.dirname):
            s2c.listdirresp.reason = 'invalid dirname specified'
            print(f'invalid dirname {c2s.listdir.dirname} specified, rejecting')
            entry_count = -1
        else:
            try:
                d: collections.abc.Iterable
                with os.scandir(self._cfg.root_dir + "/" + c2s.listdir.dirname) as d:
                    de: os.DirEntry
                    s2c.listdirresp.success = True
                    for de in d:
                        pbde = MyFS_pb2.DirEntry()
                        if de.is_dir():
                            pbde.type = MyFS_pb2.TYPE_DIR
                        elif de.is_file():
                            pbde.type = MyFS_pb2.TYPE_FILE
                        else:
                            # not a dir, not a file, skip it.
                            continue
                        pbde.entry_name = de.name
                        try:
                            statbuf = de.stat()
                            pbde.size = statbuf.st_size
                        except Exception as e:
                            print('failure stat\'ing', de.name, ':', e)
                            # skip it
                            continue
                        s2c.listdirresp.entries.append(pbde)
                        entry_count += 1
                        if s2c.ByteSize() >= self._cfg.MAX_DIR_SIZE:
                            s2c.listdirresp.final = False
                            self.send_message(s2c)
                            s2c.listdirresp.ClearField('entries')

            except Exception as e:
                s2c.listdirresp.success = False
                s2c.listdirresp.reason = 'cannot read dir %s: %s' % (c2s.listdir.dirname, e)
                print(s2c.listdirresp.reason)

        s2c.listdirresp.final = True
        self.send_message(s2c)
        if entry_count > 0:
            print(f'sent directory containing {entry_count} entries to {self._addr}')
            return True
        # else
        return False

    def _handle_list_archive(self,
                             c2s: MyFS_pb2.Client2Server) -> bool:
        print(f'got LIST_ARCHIVE from {self._addr} for file {c2s.getfile.filename}')
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.LIST_ARCHIVE_STATUS
        entry_count = 0

        if not self._valid_path(c2s.getfile.filename):
            s2c.listdirresp.success = False
            s2c.listdirresp.reason = 'invalid filename specified'
            self.send_message(s2c)
            return False  # kick them off

        fullpath = self._cfg.root_dir + '/' + c2s.getfile.filename
        if not os.path.exists(fullpath):
            s2c.listdirresp.success = False
            s2c.listdirresp.reason = 'file does not exist'
            self.send_message(s2c)
            return True

        s2c.listdirresp.success = True

        if fullpath.endswith('.zip'):
            try:
                with zipfile.ZipFile(fullpath) as z:
                    for fi in z.infolist():
                        pbde = MyFS_pb2.DirEntry()
                        pbde.entry_name = fi.filename
                        pbde.type = MyFS_pb2.TYPE_FILE
                        pbde.size = fi.file_size
                        s2c.listdirresp.entries.append(pbde)
                        entry_count += 1
                        if s2c.ByteSize() >= self._cfg.MAX_DIR_SIZE:
                            s2c.listdirresp.final = False
                            self.send_message(s2c)
                            s2c.listdirresp.ClearField('entries')
            except Exception as e:
                s2c.listdirresp.success = False
                s2c.listdirresp.reason = f'cannot read zip list ' + \
                                         f'from {c2s.getfile.filename}: {e}'
                s2c.listdirresp.ClearField('entries')
                print(s2c.listdirresp.reason)
                entry_count = -1

        elif fullpath.endswith('.tar'):
            try:
                with tarfile.open(fullpath, mode='r') as t:
                    members: list[tarfile.TarInfo] = t.getmembers()
                    for m in members:
                        pbde = MyFS_pb2.DirEntry()
                        pbde.entry_name = m.name
                        pbde.type = MyFS_pb2.TYPE_FILE
                        pbde.size = m.size
                        s2c.listdirresp.entries.append(pbde)
                        entry_count += 1
                        if s2c.ByteSize() >= self._cfg.MAX_DIR_SIZE:
                            s2c.listdirresp.final = False
                            self.send_message(s2c)
                            s2c.listdirresp.ClearField('entries')
            except Exception as e:
                s2c.listdirresp.success = False
                s2c.listdirresp.reason = f'cannot read tar file list ' + \
                                         f'from {c2s.getfile.filename}: {e}'
                s2c.listdirresp.ClearField('entries')
                print(s2c.listdirresp.reason)
                entry_count = -1
        else:
            s2c.listdirresp.success = False
            s2c.listdirresp.reason = f'file of type {c2s.getfile.filename} ' + \
                                     'is not supported'
            s2c.listdirresp.ClearField('entries')
            print(s2c.listdirresp.reason)
            entry_count = -1

        if entry_count >= 0:
            s2c.listdirresp.final = True
            s2c.listdirresp.success = True
            print(f'sent archive list containing {entry_count} entries to {self._addr}')
            self.send_message(s2c)

        return True

    def _handle_get_file(self,
                         c2s: MyFS_pb2.Client2Server) -> bool:
        print(f'got REQUEST_FILE from {self._addr} for file {c2s.getfile.filename}')
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.GET_FILE_STATUS
        if not self._valid_path(c2s.getfile.filename):
            s2c.getfilestatus.success = False
            s2c.getfilestatus.reason = 'invalid filename specified'
            self.send_message(s2c)
            return False  # kick them off
        fullpath = self._cfg.root_dir + '/' + c2s.getfile.filename
        if c2s.getfile.HasField('just_check'):
            if c2s.getfile.just_check:
                try:
                    sb = os.stat(fullpath)
                    s2c.getfilestatus.success = True
                    s2c.getfilestatus.size = sb.st_size
                except FileNotFoundError:
                    s2c.getfilestatus.success = False
                    s2c.getfilestatus.reason = 'File does not exist'
                self.send_message(s2c)
                return True
        try:
            fd = open(fullpath, 'rb')
            statbuf = os.stat(fd.fileno())
            filesize = statbuf.st_size
        except Exception as e:
            reason = 'Unable to open file: %s' % e
            print(reason)
            s2c.getfilestatus.success = False
            s2c.getfilestatus.reason = reason
            self.send_message(s2c)
            return True  # the request is ok, it's just a bum filename
        s2c.getfilestatus.success = True
        s2c.getfilestatus.size = filesize
        self.send_message(s2c)
        block_number = 0
        position = 0
        s2c.Clear()
        s2c.type = MyFS_pb2.FILE_CONTENTS
        while position < filesize:
            buf = fd.read(self._cfg.FILE_BUFFER_SIZE)
            if len(buf) <= 0:
                break
            s2c.filecontents.position = position
            position += len(buf)
            s2c.filecontents.block_number = block_number
            block_number += 1
            s2c.filecontents.data = buf
            self.send_message(s2c)
        print(f'sent {block_number} blocks of data to {self._addr}')
        return True

    def _handle_get_archive(self,
                            c2s: MyFS_pb2.Client2Server) -> bool:
        do_dnload = True
        if c2s.getarchive.HasField('requesttype'):
            if c2s.getarchive.requesttype == MyFS_pb2.ARCH_SAVE_SERVER:
                do_dnload = False
        typestr = "DWNLOAD" if do_dnload else "SAVE_SERVER"
        print(f'got GET_ARCH ({typestr}) from {self._addr} for file {c2s.getarchive.filename}')
        print(f'files requested: {c2s.getarchive.members}')
        if len(c2s.getarchive.destinations) > 0:
            print(f'local destinations: {c2s.getarchive.destinations}')
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.GET_ARCHIVE_STATUS

        if not do_dnload:
            if len(c2s.getarchive.members) != len(c2s.getarchive.destinations):
                s2c.getarchivestatus.success = False
                s2c.getarchivestatus.reason = 'members length should match destinations'
                self.send_message(s2c)
                return False  # kick them off
        else:
            if len(c2s.getarchive.destinations) > 0:
                s2c.getarchivestatus.success = False
                s2c.getarchivestatus.reason = 'type download should not have destinations'
                self.send_message(s2c)
                return False  # kick them off

        if not self._valid_path(c2s.getarchive.filename):
            s2c.getarchivestatus.success = False
            s2c.getarchivestatus.reason = f'invalid filename {c2s.getarchive.filename} specified'
            self.send_message(s2c)
            return False  # kick them off

        if not do_dnload:
            for d in c2s.getarchive.destinations:
                if not self._valid_path(d):
                    s2c.getarchivestatus.success = False
                    s2c.getarchivestatus.reason = f'invalid filename {d} specified'
                    self.send_message(s2c)
                    return False  # kick them off

        fullpath = f'{self._cfg.root_dir}/{c2s.getarchive.filename}'
        if not os.path.exists(fullpath):
            s2c.getarchivestatus.success = False
            s2c.getarchivestatus.reason = 'file does not exist'
            self.send_message(s2c)
            return True

        if fullpath.endswith('.zip'):
            try:
                zf = zipfile.ZipFile(fullpath)
            except Exception as e:
                s2c.getarchivestatus.success = False
                s2c.getarchivestatus.reason = f'failed to parse zip: {e}'
                self.send_message(s2c)
                return True
            # first validate if all the filenames are in the zip
            for fn in c2s.getarchive.members:
                if fn not in zf.namelist():
                    s2c.getarchivestatus.success = False
                    s2c.getarchivestatus.reason = f'file {fn} not in zip file'
                    self.send_message(s2c)
                    return True
                if do_dnload:
                    s2c.getarchivestatus.sizes.append(zf.getinfo(fn).file_size)
            s2c.getarchivestatus.success = True
            if not do_dnload:
                for ind in range(len(c2s.getarchive.members)):
                    dest = c2s.getarchive.destinations[ind]
                    destfull = f'{self._cfg.root_dir}/{dest}'
                    body = zf.read(c2s.getarchive.members[ind])
                    try:
                        with open(destfull, 'wb') as fd:
                            fd.write(body)
                    except Exception as e:
                        s2c.getarchivestatus.success = False
                        s2c.getarchivestatus.reason = f'writing dest {dest}: {e}'
                        break
                self.send_message(s2c)
                return True
            self.send_message(s2c)
            s2c.Clear()
            s2c.type = MyFS_pb2.FILE_CONTENTS
            for fn in c2s.getarchive.members:
                body = zf.read(fn)
                remaining = len(body)
                print(f'fetching {fn} from {c2s.getfile.filename}'
                      f' got {remaining} bytes')
                block_number = 0
                position = 0
                s2c.Clear()
                s2c.type = MyFS_pb2.FILE_CONTENTS
                while remaining > 0:
                    piecelen = self._cfg.FILE_BUFFER_SIZE
                    if piecelen > remaining:
                        piecelen = remaining
                    s2c.filecontents.position = position
                    s2c.filecontents.block_number = block_number
                    s2c.filecontents.data = body[position:position+piecelen]
                    self.send_message(s2c)
                    position += piecelen
                    remaining -= piecelen
                    block_number += 1
                print(f'sent {block_number} blocks of data to {self._addr}')

        elif fullpath.endswith('.tar'):
            try:
                tf = tarfile.TarFile(fullpath, mode='r')
            except Exception as e:
                s2c.getarchivestatus.success = False
                s2c.getarchivestatus.reason = f'failed to parse tar: {e}'
                self.send_message(s2c)
                return True
            # first validate if all the filenames are in the zip
            members: list[tarfile.TarInfo] = tf.getmembers()

            for fn in c2s.getarchive.members:
                found = False
                for m in members:
                    if m.name == fn:
                        if do_dnload:
                            s2c.getarchivestatus.sizes.append(m.size)
                        found = True
                        break
                if not found:
                    s2c.getarchivestatus.success = False
                    s2c.getarchivestatus.reason = f'file {fn} not in tar file'
                    self.send_message(s2c)
                    return True

            s2c.getarchivestatus.success = True
            if not do_dnload:
                for ind in range(len(c2s.getarchive.members)):
                    src = c2s.getarchive.members[ind]
                    dest = c2s.getarchive.destinations[ind]
                    destfull = f'{self._cfg.root_dir}/{dest}'
                    body = tf.extractfile(src).read()
                    try:
                        with open(destfull, 'wb') as fd:
                            fd.write(body)
                    except Exception as e:
                        s2c.getarchivestatus.success = False
                        s2c.getarchivestatus.reason = f'writing dest {dest}: {e}'
                        break
                self.send_message(s2c)
                return True
            self.send_message(s2c)
            s2c.Clear()
            s2c.type = MyFS_pb2.FILE_CONTENTS
            for fn in c2s.getarchive.members:
                body = tf.extractfile(fn).read()
                remaining = len(body)
                print(f'fetching {fn} from {c2s.getfile.filename}'
                      f' got {remaining} bytes')
                block_number = 0
                position = 0
                s2c.Clear()
                s2c.type = MyFS_pb2.FILE_CONTENTS
                while remaining > 0:
                    piecelen = self._cfg.FILE_BUFFER_SIZE
                    if piecelen > remaining:
                        piecelen = remaining
                    s2c.filecontents.position = position
                    s2c.filecontents.block_number = block_number
                    s2c.filecontents.data = body[position:position+piecelen]
                    self.send_message(s2c)
                    position += piecelen
                    remaining -= piecelen
                    block_number += 1
                print(f'sent {block_number} blocks of data to {self._addr}')

        else:
            s2c.getarchivestatus.success = False
            s2c.getarchivestatus.reason = f'file type not supported'
            self.send_message(s2c)
            return True

        return True

    def _handle_put_file(self,
                         c2s: MyFS_pb2.Client2Server) -> bool:
        print(f'got PUT_FILE from {self._addr} for file {c2s.putfile.filename}')
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.PUT_FILE_STATUS
        if not self._valid_path(c2s.putfile.filename):
            s2c.putfilestatus.success = False
            s2c.putfilestatus.reason = 'invalid filename specified'
            self.send_message(s2c)
            return False  # kick them off
        fullpath = self._cfg.root_dir + '/' + c2s.putfile.filename
        try:
            self._put_fd = open(fullpath, 'wb')
        except Exception as e:
            s2c.putfilestatus.success = False
            s2c.putfilestatus.reason = f'cannot open output file: {e}'
            self.send_message(s2c)
            return False  # kick them off
        self._put_remaining = c2s.putfile.size
        self._put_in_progress = True
        s2c.putfilestatus.success = True
        self.send_message(s2c)
        return True

    def _handle_file_contents(self,
                              c2s: MyFS_pb2.Client2Server) -> bool:
        if self._cfg.debug_text:
            print(f'got FILE_DATA block {c2s.filecontents.block_number} ' +
                  f'of size {len(c2s.filecontents.data)}')
        if not self._put_in_progress:
            print('no PUT is in progress! discarding message')
            return False
        try:
            self._put_fd.write(c2s.filecontents.data)
        except Exception as e:
            print(f'PUT of file contents failed: {e}')
            return False
        self._put_remaining -= len(c2s.filecontents.data)
        if self._put_remaining == 0:
            s2c = MyFS_pb2.Server2Client()
            s2c.type = MyFS_pb2.PUT_FILE_STATUS
            s2c.putfilestatus.success = True
            self.send_message(s2c)
            self._put_fd.close()
            self._put_fd = None
            self._put_in_progress = False
        return True

    def _handle_delete(self,
                       c2s: MyFS_pb2.Client2Server) -> bool:
        print(f'got DELETE from {self._addr} for file {c2s.getfile.filename}')
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.DELETE_STATUS
        if not self._valid_path(c2s.getfile.filename):
            s2c.putfilestatus.success = False
            s2c.putfilestatus.reason = 'invalid filename specified'
            self.send_message(s2c)
            return False  # kick them off
        fullpath = self._cfg.root_dir + '/' + c2s.getfile.filename
        try:
            os.unlink(fullpath)
        except Exception as e:
            s2c.putfilestatus.success = False
            s2c.putfilestatus.reason = f'failure removing: {e}'
            self.send_message(s2c)
            return False
        s2c.putfilestatus.success = True
        self.send_message(s2c)
        return True

    def _handle_mkdir(self,
                      c2s: MyFS_pb2.Client2Server) -> bool:
        print(f'got MKDIR from {self._addr} for dir {c2s.getfile.filename}')
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.MKDIR_STATUS
        if not self._valid_path(c2s.getfile.filename):
            s2c.putfilestatus.success = False
            s2c.putfilestatus.reason = 'invalid filename specified'
            self.send_message(s2c)
            return False  # kick them off
        fullpath = self._cfg.root_dir + '/' + c2s.getfile.filename
        try:
            os.mkdir(fullpath)
        except Exception as e:
            s2c.putfilestatus.success = False
            s2c.putfilestatus.reason = f'failure making dir: {e}'
            self.send_message(s2c)
            return False
        s2c.putfilestatus.success = True
        self.send_message(s2c)
        return True

    def _handle_rmdir(self,
                      c2s: MyFS_pb2.Client2Server) -> bool:
        print(f'got RMDIR from {self._addr} for dir {c2s.getfile.filename}')
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.RMDIR_STATUS
        if not self._valid_path(c2s.getfile.filename):
            s2c.putfilestatus.success = False
            s2c.putfilestatus.reason = 'invalid filename specified'
            self.send_message(s2c)
            return False  # kick them off
        fullpath = self._cfg.root_dir + '/' + c2s.getfile.filename
        try:
            os.rmdir(fullpath)
        except Exception as e:
            s2c.putfilestatus.success = False
            s2c.putfilestatus.reason = f'failure removing dir: {e}'
            self.send_message(s2c)
            return False
        s2c.putfilestatus.success = True
        self.send_message(s2c)
        return True

    @staticmethod
    def _rmtree(path: str) -> (bool, str | None):
        if not os.path.exists(path):
            # well ..... it's removed ....
            return True, None
        todo = [path]
        rmdirtodo = [path]
        while len(todo) > 0:
            p = todo[0]
            todo = todo[1:]
            with os.scandir(p) as d:
                de: os.DirEntry
                for de in d:
                    e = f'{p}/{de.name}'
                    if de.is_dir():
                        todo.append(e)
                        rmdirtodo.insert(0, e)
                    elif de.is_file():
                        os.unlink(e)
        removedir: str
        try:
            for removedir in rmdirtodo:
                os.rmdir(removedir)
        except OSError as e:
            reason = f'RMDIR {path} error: {e}'
            print(reason)
            return False, reason
        return True, None

    def _handle_remove_tree(self,
                            c2s: MyFS_pb2.Client2Server) -> bool:
        print(f'got REMOVETREE from {self._addr} for {c2s.getfile.filename}')
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.REMOVE_TREE_STATUS
        if not self._valid_path(c2s.getfile.filename):
            s2c.putfilestatus.success = False
            s2c.putfilestatus.reason = 'invalid filename specified'
            self.send_message(s2c)
            return False  # kick them off
        fullpath = self._cfg.root_dir + '/' + c2s.getfile.filename
        success, reason = self._rmtree(fullpath)
        s2c.putfilestatus.success = success
        if reason:
            s2c.putfilestatus.reason = reason
        self.send_message(s2c)
        return True


class MyFSTCPServer(TcpServer):
    _cfg: MyFSConfig

    def __init__(self, cfg: MyFSConfig):
        self._cfg = cfg
        super().__init__(cfg.server_port, self._new_conn_callback)

    def _new_conn_callback(self, new_s, addr):
        # instantiating this is enough, the PbConnection constructor
        # spawns a thread which holds the object reference. we don't
        # need to keep the object.
        MyFSServer(self._cfg, new_s, addr)
