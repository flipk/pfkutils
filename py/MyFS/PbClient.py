
from MyFS import PbServer
from MyFS import MyFS_pb2
import socket
from google.protobuf.internal.containers import RepeatedCompositeFieldContainer


class PbClient:
    SERVER_PORT: int = PbServer.PbServer.SERVER_PORT
    MAX_MSG_LEN: int = PbServer.PbServer.MAX_MSG_LEN
    _s: socket.socket
    _ok: bool
    _recvdata: bytes

    def __init__(self, addr: str):
        self._ok = False
        self._s = socket.socket()
        try:
            self._s.connect((addr, PbClient.SERVER_PORT))
            self._recvdata = bytes()
            self._ok = True
        except Exception as e:
            print('PbClient connect exception:', e)

    @property
    def ok(self):
        return self._ok

    def _send_message(self, msg: MyFS_pb2.Client2Server):
        body = msg.SerializeToString()
        length = len(body)  # m.ByteSize()
        encoded = length.to_bytes(2, byteorder='big') + body
        # print('sending message:')
        # print(msg)
        # print('encoded as:')
        # print(" ".join("{:02x}".format(c) for c in encoded))
        self._s.send(encoded)

    @staticmethod
    def _decode_msg(msg: bytes) -> (bool, MyFS_pb2.Server2Client):
        s2c = MyFS_pb2.Server2Client()
        try:
            s2c.ParseFromString(msg)
        except Exception as e:
            print('exception parsing msg:', e)
            print('could not parse:',
                  " ".join("{:02x}".format(c) for c in msg))
            return False, None
        return True, s2c

    def _get_message(self) -> (bool, MyFS_pb2.Server2Client):
        while True:
            if len(self._recvdata) > 2:
                length = int.from_bytes(self._recvdata[0:2], byteorder='big')
                if length > self.MAX_MSG_LEN:
                    # bogusly long message
                    return False, None
                if (len(self._recvdata) + 2) >= length:
                    success, s2c = self._decode_msg(self._recvdata[2:length + 2])
                    if not success:
                        return False, None
                    self._recvdata = self._recvdata[length + 2:]
                    # print('done processing a message of length', length,
                    #       'leaving a buffer of size', len(self._recvdata))
                    return True, s2c
            buf = self._s.recv(4096)
            self._recvdata += buf

    def login(self, user: str, password: str) -> bool:
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.MessageType.LOGIN
        c2s.login.user = user
        c2s.login.password = password
        self._send_message(c2s)
        success, s2c = self._get_message()
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
            (bool, str, RepeatedCompositeFieldContainer):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.LIST_DIR
        c2s.listdir.dirname = dirname
        self._send_message(c2s)
        success, s2c = self._get_message()
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
        return True, '', s2c.listdirresp.entries

    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def request_file(self, filename: str) -> (bool, str, bytes):
        c2s = MyFS_pb2.Client2Server()
        c2s.type = MyFS_pb2.REQUEST_FILE
        c2s.requestfile.filename = filename
        filebody = bytes()
        self._send_message(c2s)
        success, s2c = self._get_message()
        if not success:
            reason = 'failure getting response to file request'
            print(reason)
            return False, reason, None
        if s2c.type != MyFS_pb2.REQUEST_FILE_STATUS:
            reason = 'protocol error, wrong response'
            print(reason, ':', s2c)
            return False, reason, None
        if not s2c.requestfilestatus.success:
            print('file request failed:', s2c.requestfilestatus.reason)
            return False, s2c.requestfilestatus.reason, None
        filesize = s2c.requestfilestatus.size
        # now expect a stream of FILE_CONTENTS
        while filesize > 0:
            success, s2c = self._get_message()
            if s2c.type != MyFS_pb2.FILE_CONTENTS:
                reason = 'protocol error, expected file contents'
                print(reason, ':', s2c)
                return False, reason, None
            filebody += s2c.filecontents.data
            filesize -= len(s2c.filecontents.data)
            print('got block', s2c.filecontents.block_number,
                  'at position', s2c.filecontents.position,
                  'of length', len(s2c.filecontents.data))
        return True, '', filebody
