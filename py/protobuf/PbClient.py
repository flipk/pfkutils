

query replace
import MyFS_pb2
import socket
# from google.protobuf import message
import PbServer


class PbClient:
    SERVER_PORT: int = PbServer.PbServer.SERVER_PORT
    MAX_MSG_LEN: int = PbServer.PbServer.MAX_MSG_LEN
    _s: socket.socket
    _ok: bool

    def __init__(self, addr: str):
        self._ok = False
        self._s = socket.socket()
        try:
            self._s.connect((addr, PbClient.SERVER_PORT))
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
        self._s.send(encoded)

    def _get_message(self) -> (bool, MyFS_pb2.Server2Client):
        recvdata = bytes()
        waiting = True
        ret = False
        s2c = MyFS_pb2.Server2Client()
        while waiting:
            buf = self._s.recv(4096)
            if len(buf) <= 0:
                print('connection to server died')
                break
            recvdata += buf
            if len(recvdata) > 2:
                length = int.from_bytes(recvdata[0:2], byteorder='big')
                if length > self.MAX_MSG_LEN:
                    print('message too big')
                    break
                if (len(recvdata) + 2) >= length:
                    msg = recvdata[2:2 + length]
                    try:
                        s2c.ParseFromString(msg)
                        ret = True
                        waiting = False
                    except Exception as e:
                        print('exception parsing msg:', e)
                        print('could not parse:',
                              " ".join("{:02x}".format(c) for c in msg))
                    recvdata = recvdata[2 + length:]
        return ret, s2c

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
        if not s2c.loginresp.success:
            print('login rejected, reason:', s2c.loginresp.reason)
            return False
        print('login success, message:', s2c.loginresp.reason)
        return True

    def test(self) -> bool:
        return self.login('pfk', 'password')
