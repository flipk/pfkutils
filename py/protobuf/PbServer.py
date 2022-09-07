
from typing import Dict, Callable

import MyFS_pb2
import threading
import socket
# from google.protobuf import message
from MyFS_pb2 import Client2Server


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
    _handlers: Dict[int, Callable[[_ConnData, Client2Server], bool]]
    SERVER_PORT: int = 21005
    MAX_MSG_LEN: int = 65536
    _lck: threading.Lock
    _server_port: socket.socket
    _ok: bool

    def __init__(self):
        self._ok = False
        try:
            self._lck = threading.Lock()
            self._server_port = socket.socket()
            self._server_port.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
            self._server_port.bind(('', self.SERVER_PORT))
            self._handlers = {
                MyFS_pb2.LOGIN: self._handle_login,
                MyFS_pb2.REQUEST_FILE: self._handle_request_file
            }
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
        length = len(body)  # msg.ByteSize()
        encoded = length.to_bytes(2, byteorder='big') + body
        cd.s.send(encoded)

    # noinspection PyMethodMayBeStatic
    def _handle_login(self, cd: _ConnData, c2s: MyFS_pb2.Client2Server) -> bool:
        print('got LOGIN message', c2s)
        ret = False
        s2c = MyFS_pb2.Server2Client()
        s2c.type = MyFS_pb2.LOGIN_RESP
        if c2s.login.user == "pfk" and c2s.login.password == "password":
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

    # noinspection PyUnusedLocal,PyMethodMayBeStatic
    def _handle_request_file(self, cd: _ConnData,
                             msg: MyFS_pb2.Client2Server) -> bool:
        print('got REQUEST_FILE message', msg)
        # TODO FILL OUT
        return False

    # returns False if connection should be killed.
    def _process_msg(self, cd: _ConnData, length: int) -> bool:
        ret = False
        msg = cd.recvdata[2:2 + length]
        c2s = MyFS_pb2.Client2Server()
        try:
            c2s.ParseFromString(msg)
            handler = self._handlers.get(c2s.type, None)
            if handler:
                # LOGIN is the only message type allowed
                # if not yet authenticated.
                if cd.authenticated or \
                   c2s.type == MyFS_pb2.MessageType.LOGIN:
                    ret = handler(cd, c2s)
                    if not ret:
                        print('bogus message not handled')
                else:
                    print('unauthorized message attempted', c2s)
            else:
                print('unhandled message!', c2s)
        except Exception as e:
            print('process_msg exception:', e)
            print('could not parse msg:',
                  " ".join("{:02x}".format(c) for c in msg))
        return ret

    # returns False if connection should be killed.
    def _process_data(self, cd: _ConnData) -> bool:
        print('now have', len(cd.recvdata), 'bytes')
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
                print('protocol error, dropping conn from', addr)
                break
        s.close()

    def serve(self):
        self._server_port.listen(1)
        while True:
            new_s, addr = self._server_port.accept()
            t = threading.Thread(target=self._handle_connection,
                                 args=[new_s, addr], daemon=True)
            t.start()
