import socket
from typing import Callable
from PbConnection import PbConnection
from MyFS_Config import MyFSConfig
import MyFS_pb2


class MyFSClient(PbConnection):
    _cfg: MyFSConfig
    _svrok: bool

    def __init__(self, cfg: MyFSConfig):
        self._cfg = cfg
        self._svrok = False
        addr = (cfg.addr, cfg.server_port)
        sock = socket.socket()
        try:
            sock.connect(addr)
            super().__init__(sock, sock, addr,
                             MyFS_pb2.Server2Client(),
                             None, None,
                             self._cfg.debug_text,
                             self._cfg.debug_binary)
        except Exception as e:
            print(f'failed to connect to {addr}: {e}')

    def login(self) -> bool:
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.MessageType.LOGIN
        c2s.login.user = self._cfg.username
        c2s.login.password = self._cfg.password
        self.send_message(c2s)
        success, s2c = self.get_message()
        if not success:
            print('failure getting response message')
            return False
        if s2c.type != MyFS_pb2.LOGIN_RESP:
            print('protocol error, wrong response:', s2c)
            return False
        if not s2c.loginresp.success:
            print('login rejected, reason:', s2c.loginresp.reason)
            return False
        print('login success, message:', s2c.loginresp.reason)
        return True

    def list_dir(self, dirname: str) -> \
            (bool, str, list[MyFS_pb2.DirEntry]):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.LIST_DIR
        c2s.listdir.dirname = dirname
        self.send_message(c2s)
        ret = []
        s2c: MyFS_pb2.Server2Client
        while True:
            success, s2c = self.get_message()
            if not success:
                print('failure getting list dir response')
                return False, 'failure getting response message', None
            if s2c.type != MyFS_pb2.LIST_DIR_RESP:
                print('protocol error, wrong response:', s2c)
                return False
            if not s2c.listdirresp.success:
                print('couldnt list dir', dirname, 'because:',
                      s2c.listdirresp.reason)
                return False, s2c.listdirresp.reason, None
            for de in s2c.listdirresp.entries:
                ret.append(de)
            if s2c.listdirresp.final:
                break
        return True, '', ret

    def list_archive(self, filename: str) -> \
            (bool, str, list[MyFS_pb2.DirEntry]):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.LIST_ARCHIVE
        c2s.getfile.filename = filename
        self.send_message(c2s)
        ret = []
        while True:
            s2c: MyFS_pb2.Server2Client
            success, s2c = self.get_message()
            if not success:
                reason = 'failure getting response to ARCHIVE_LIST request'
                print(reason)
                return False, reason, ret
            if s2c.type != MyFS_pb2.LIST_ARCHIVE_STATUS:
                reason = 'protocol error, wrong response'
                print(reason)
                return False, reason, ret
            if not s2c.listdirresp.success:
                print(f'list archive {filename} failed: {s2c.listdirresp.reason}')
                return False, s2c.listdirresp.reason, ret
            for de in s2c.listdirresp.entries:
                ret.append(de)
            if s2c.listdirresp.final:
                break
        return True, '', ret

    def get_file(self, filename: str,
                 just_check: bool = False,
                 status_func: Callable[[int], None] = None) -> (bool, str, bytes | int):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.GET_FILE
        c2s.getfile.filename = filename
        c2s.getfile.just_check = just_check
        filebody = bytes()
        self.send_message(c2s)
        s2c: MyFS_pb2.Server2Client
        success, s2c = self.get_message()
        if not success:
            reason = 'failure getting response to file request'
            print(reason)
            return False, reason, None
        if s2c.type != MyFS_pb2.GET_FILE_STATUS:
            reason = 'protocol error, wrong response'
            print(reason, ':', s2c)
            return False, reason, None
        if not s2c.getfilestatus.success:
            if not just_check:
                print('file request failed:', s2c.getfilestatus.reason)
            return False, s2c.getfilestatus.reason, None
        if just_check:
            return True, '', s2c.getfilestatus.size
        filesize = s2c.getfilestatus.size
        # now expect a stream of FILE_CONTENTS
        while filesize > 0:
            success, s2c = self.get_message()
            if not success:
                reason = 'failure getting the next FILE_CONTENTS'
                return False, reason, None
            if s2c.type != MyFS_pb2.FILE_CONTENTS:
                reason = 'protocol error, expected file contents'
                print(reason, ':', s2c)
                return False, reason, None
            pktsize = len(s2c.filecontents.data)
            filebody += s2c.filecontents.data
            filesize -= pktsize
            if status_func:
                status_func(pktsize)
            # print('got block', s2c.filecontents.block_number,
            #       'at position', s2c.filecontents.position,
            #       'of length', len(s2c.filecontents.data))
        return True, '', filebody

    def get_archive(self,
                    filename: str,
                    members: list[str],
                    destinations: list[str] | None = None,
                    status_func: Callable[[int], None] = None) -> \
            (bool, str, list[bytes] | None):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.GET_ARCHIVE
        if not destinations:
            c2s.getarchive.requesttype = MyFS_pb2.ARCH_DOWNLOAD
        else:
            if len(members) != len(destinations):
                return False, 'destinations should be None or same len as members'
            c2s.getarchive.requesttype = MyFS_pb2.ARCH_SAVE_SERVER
            for d in destinations:
                c2s.getarchive.destinations.append(d)
        c2s.getarchive.filename = filename
        filebodies = []
        for m in members:
            c2s.getarchive.members.append(m)
        self.send_message(c2s)
        success, s2c = self.get_message()
        if not success:
            reason = 'failure getting response to GET_ARCHIVE request'
            print(reason)
            return False, reason, None
        if s2c.type != MyFS_pb2.GET_ARCHIVE_STATUS:
            reason = 'protocol error, wrong response'
            print(reason)
            return False, reason, None
        if not s2c.getarchivestatus.success:
            print(f'archive get failed: {s2c.getarchivestatus.reason}')
            return False, s2c.getarchivestatus.reason, None
        if destinations:
            return True, '', None
        else:
            fileind = 0
            for filesize in s2c.getarchivestatus.sizes:
                filebody = bytes()
                while filesize > 0:
                    success, s2c = self.get_message()
                    if not success:
                        reason = 'failure getting the next FILE_CONTENTS'
                        return False, reason, None
                    if s2c.type != MyFS_pb2.FILE_CONTENTS:
                        reason = 'protocol error, expected file contents'
                        print(reason, ':', s2c)
                        return False, reason, None
                    # print(f'got block {s2c.filecontents.block_number} of'
                    #       f' file #{fileind}')
                    pktsize = len(s2c.filecontents.data)
                    filebody += s2c.filecontents.data
                    filesize -= pktsize
                    if status_func:
                        status_func(pktsize)
                filebodies.append(filebody)
                fileind += 1
            return True, '', filebodies

    def put_file(self, filename: str, filebody: bytes) -> (bool, str):
        remaining = len(filebody)
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.PUT_FILE
        c2s.putfile.filename = filename
        c2s.putfile.size = remaining
        self.send_message(c2s)
        success, s2c = self.get_message()
        if not success:
            reason = 'failure getting response to PUT request'
            print(reason)
            return False, reason
        if s2c.type != MyFS_pb2.PUT_FILE_STATUS:
            reason = 'protocol error, wrong response'
            print(reason)
            return False, reason
        if not s2c.putfilestatus.success:
            print(f'file put failed: {s2c.putfilestatus.reason}')
            return False, s2c.putfilestatus.reason
        # it said ok, so proceed to send a stream of FILE_CONTENTS
        c2s.type = MyFS_pb2.FILE_CONTENTS
        c2s.ClearField('putfile')
        position = 0
        block_number = 0
        while remaining > 0:
            this_size = remaining
            if this_size > self._cfg.FILE_BUFFER_SIZE:
                this_size = self._cfg.FILE_BUFFER_SIZE
            # print(f'sending block {block_number} of size {this_size}')
            c2s.filecontents.data = filebody[position:position+this_size]
            c2s.filecontents.position = position
            c2s.filecontents.block_number = block_number
            self.send_message(c2s)
            remaining -= this_size
            position += this_size
            block_number += 1
        success, s2c = self.get_message()
        if not success:
            reason = 'file put protocol error 1'
            print(reason)
            return False, reason
        if s2c.type != MyFS_pb2.PUT_FILE_STATUS:
            reason = 'file put protocol error 2'
            print(reason)
            return False, reason
        if not s2c.putfilestatus.success:
            print(f'put error from server: {s2c.putfilestatus.reason}')
            return False, s2c.putfilestatus.reason
        return True, ''

    def delete_file(self, filename: str) -> (bool, str):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.DELETE
        c2s.getfile.filename = filename
        self.send_message(c2s)
        success, s2c = self.get_message()
        if not success:
            reason = 'failure getting response to DELETE request'
            print(reason)
            return False, reason
        if s2c.type != MyFS_pb2.DELETE_STATUS:
            reason = 'protocol error, wrong response'
            print(reason)
            return False, reason
        if not s2c.putfilestatus.success:
            print(f'delete file {filename} failed: {s2c.putfilestatus.reason}')
            return False, s2c.putfilestatus.reason
        return True, ''

    def make_dir(self, dirname: str) -> (bool, str):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.MKDIR
        c2s.getfile.filename = dirname
        self.send_message(c2s)
        success, s2c = self.get_message()
        if not success:
            reason = 'failure getting response to MKDIR request'
            print(reason)
            return False, reason
        if s2c.type != MyFS_pb2.MKDIR_STATUS:
            reason = 'protocol error, wrong response'
            print(reason)
            return False, reason
        if not s2c.putfilestatus.success:
            print(f'mkdir {dirname} failed: {s2c.putfilestatus.reason}')
            return False, s2c.putfilestatus.reason
        return True, ''

    def remove_dir(self, dirname: str) -> (bool, str):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.RMDIR
        c2s.getfile.filename = dirname
        self.send_message(c2s)
        success, s2c = self.get_message()
        if not success:
            reason = 'failure getting response to RMDIR request'
            print(reason)
            return False, reason
        if s2c.type != MyFS_pb2.RMDIR_STATUS:
            reason = 'protocol error, wrong response'
            print(reason)
            return False, reason
        if not s2c.putfilestatus.success:
            print(f'file delete failed: {s2c.putfilestatus.reason}')
            return False, s2c.putfilestatus.reason
        return True, ''

    def remove_tree(self, dirname: str) -> (bool, str):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.REMOVE_TREE
        c2s.getfile.filename = dirname
        self.send_message(c2s)
        success, s2c = self.get_message()
        if not success:
            reason = 'failure getting response to REMOVE_TREE request'
            print(reason)
            return False, reason
        if s2c.type != MyFS_pb2.REMOVE_TREE_STATUS:
            reason = 'protocol error, wrong response'
            print(reason)
            return False, reason
        if not s2c.putfilestatus.success:
            print(f'recursive delete failed: {s2c.putfilestatus.reason}')
            return False, s2c.putfilestatus.reason
        return True, ''
