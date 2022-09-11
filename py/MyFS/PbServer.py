
import os
from typing import Dict, Callable
from MyFS import MyFS_pb2
import threading
import socket


class _ConnData:
    """
    context structure for each connection; this way, multiple threads
    can manage multiple connections but each one has its own recvdata
    accumulation buffer.
    """
    s: socket.socket
    recvdata: bytes
    authenticated: bool

    def __init__(self, s: socket.socket):
        self.recvdata = bytes()
        self.s = s
        self.authenticated = False


class PbServer:
    # the key type for _handlers is actually MyFS_pb2.MessageType enum
    _handlers: Dict[int, Callable[[_ConnData, MyFS_pb2.Client2Server], bool]]
    SERVER_PORT: int = 21005
    MAX_MSG_LEN: int = 65536
    FILE_BUFFER_SIZE: int = 16384
    _lck: threading.Lock
    _server_port: socket.socket
    _ok: bool
    _root_dir: str
    _username: str
    _password: str

    def __init__(self, root_dir: str, username: str, password: str):
        self._ok = False
        try:
            self._lck = threading.Lock()
            self._server_port = socket.socket()
            self._server_port.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
            self._server_port.bind(('', self.SERVER_PORT))
            self._username = username
            self._password = password
            self._handlers = {
                MyFS_pb2.LOGIN: self._handle_login,
                MyFS_pb2.REQUEST_FILE: self._handle_request_file,
                MyFS_pb2.LIST_DIR: self._handle_list_dir,
            }
            self._root_dir = root_dir
            self._ok = True
        except Exception as e:
            print("PbServer init exception:", e)
            raise

    @property
    def ok(self) -> bool:
        return self._ok

    @staticmethod
    def _send_message(cd: _ConnData, msg: MyFS_pb2.Server2Client):
        body = msg.SerializeToString()
        length = len(body)  # m.ByteSize()
        encoded = length.to_bytes(2, byteorder='big') + body
        # print('sending message:')
        # print(msg)
        # print('encoded as:')
        # print(" ".join("{:02x}".format(c) for c in encoded))
        cd.s.send(encoded)

    # noinspection PyMethodMayBeStatic
    def _handle_login(self, cd: _ConnData, c2s: MyFS_pb2.Client2Server) -> bool:
        print('got LOGIN message for user', c2s.login.user)
        ret = False
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.LOGIN_RESP
        if c2s.login.user == self._username and \
           c2s.login.password == self._password:
            cd.authenticated = True
            print('user authenticated!')
            s2c.loginresp.success = True
            s2c.loginresp.reason = 'successful login, welcome'
            ret = True
        else:
            print("authentication failed, kicking off")
            s2c.loginresp.success = False
            s2c.loginresp.reason = 'invalid username or password'
        self._send_message(cd, s2c)
        return ret

    @staticmethod
    def _valid_path(path: str):
        if path[0] == '/':
            # absolute paths not permitted
            return False
        if path.find('..') >= 0:
            # escaping from root dir not permitted
            return False
        return True

    def _handle_list_dir(self, cd: _ConnData,
                         c2s: MyFS_pb2.Client2Server) -> bool:
        print('got LIST_DIR message for dir', c2s.listdir.dirname)
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.LIST_DIR_RESP
        s2c.listdirresp.success = False
        if not self._valid_path(c2s.listdir.dirname):
            s2c.listdirresp.reason = 'invalid dirname specified'
        else:
            de = MyFS_pb2.DirEntry()
            try:
                with os.scandir(self._root_dir + "/" + c2s.listdir.dirname) as d:
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
                            print('failure statting', de.name, ':', e)
                            # skip it
                            continue
                        s2c.listdirresp.entries.append(pbde)
            except Exception as e:
                s2c.listdirresp.success = False
                s2c.listdirresp.reason = 'cannot read dir %s: %s' % (c2s.listdir.dirname, e)
                print(s2c.listdirresp.reason)
        self._send_message(cd, s2c)
        print('sent directory containing', len(s2c.listdirresp.entries), 'entries')
        return True

    def _handle_request_file(self, cd: _ConnData,
                             c2s: MyFS_pb2.Client2Server) -> bool:
        print('got REQUEST_FILE message for file', c2s.requestfile.filename)
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.REQUEST_FILE_STATUS
        if not self._valid_path(c2s.requestfile.filename):
            s2c.requestfilestatus.success = False
            s2c.requestfilestatus.reason = 'invalid filename specified'
            self._send_message(cd, s2c)
            return False  # kick them off
        else:
            fullpath = self._root_dir + '/' + c2s.requestfile.filename
            try:
                fd = open(fullpath, 'rb')
                statbuf = os.stat(fd.fileno())
                filesize = statbuf.st_size
            except Exception as e:
                reason = 'Unable to open file: %s' % e
                print(reason)
                s2c.requestfilestatus.success = False
                s2c.requestfilestatus.reason = reason
                self._send_message(cd, s2c)
                return True  # the request is ok, it's just a bum filename
            s2c.requestfilestatus.success = True
            s2c.requestfilestatus.size = filesize
            self._send_message(cd, s2c)
            block_number = 0
            position = 0
            s2c.Clear()
            s2c.type = MyFS_pb2.FILE_CONTENTS
            while position < filesize:
                buf = fd.read(PbServer.FILE_BUFFER_SIZE)
                if len(buf) <= 0:
                    break
                s2c.filecontents.position = position
                position += len(buf)
                s2c.filecontents.block_number = block_number
                block_number += 1
                s2c.filecontents.data = buf
                self._send_message(cd, s2c)
            print('sent', block_number, 'blocks of data to client')
        return True

    # returns False if connection should be killed.
    def _process_msg(self, cd: _ConnData, length: int) -> bool:
        ret = False
        msg = cd.recvdata[2:2 + length]
        c2s = MyFS_pb2.Client2Server()
        try:
            # print('got bytes:')
            # print(" ".join("{:02x}".format(c) for c in msg))
            c2s.ParseFromString(msg)
            # print('decoded message:', c2s)
            handler = self._handlers.get(c2s.type, None)
            if handler:
                # LOGIN is the only message type allowed
                # if not yet authenticated.
                if cd.authenticated or \
                   c2s.type == MyFS_pb2.MessageType.LOGIN:
                    ret = handler(cd, c2s)
                    if not ret:
                        print('message handler said to kill it')
                else:
                    print('unauthorized message attempted', c2s)
            else:
                print('unhandled message:', c2s)
        except Exception as e:
            print('process_msg exception:', e)
            print('could not parse msg:',
                  " ".join("{:02x}".format(c) for c in msg))
            raise
        return ret

    # returns False if connection should be killed.
    def _process_data(self, cd: _ConnData) -> bool:
        # print('now have', len(cd.recvdata), 'bytes')
        while len(cd.recvdata) > 2:
            length = int.from_bytes(cd.recvdata[0:2], byteorder='big')
            if length > self.MAX_MSG_LEN:
                # bogusly sent message, or maybe garbage.
                return False
            if length > (len(cd.recvdata) + 2):
                # not enough for a message, that's ok, more should come.
                break
            if not self._process_msg(cd, length):
                # if we couldn't decode it, somethings bogus,
                # just dump the connection.
                return False
            # not the most efficient, but it's ok.
            cd.recvdata = cd.recvdata[2+length:]
            # print('done processing a message of length', length,
            #       'leaving a buffer of size', len(cd.recvdata))
        return True

    def _handle_connection(self, s: socket.socket, addr):
        print('new connection from', addr)
        cd = _ConnData(s)
        while True:
            buf = s.recv(4096)
            if len(buf) <= 0:
                print('connection from', addr, 'died')
                break
            cd.recvdata += buf
            if not self._process_data(cd):
                print('dropping conn from', addr)
                break
        s.close()

    def serve(self):
        self._server_port.listen(1)
        print('PbServer ready to handle new connections on port',
              self.SERVER_PORT)
        while True:
            new_s, addr = self._server_port.accept()
            t = threading.Thread(target=self._handle_connection,
                                 args=[new_s, addr], daemon=True)
            t.start()
