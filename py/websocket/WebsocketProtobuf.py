
from __future__ import annotations
import queue
import random
import threading
from typing import Union
from enum import Enum
# noinspection PyPackageRequirements
from google.protobuf import message as pbmsg
import websockets.sync.client as wsclient
import websockets.sync.server as wsserver
from typing import Callable


# for documentation on sync websocket api, read:
# https://websockets.readthedocs.io/en/stable/reference/sync/client.html
# https://websockets.readthedocs.io/en/stable/reference/sync/server.html

class withpb:
    """helper class to allow the use of 'with' statements on objects that dont
    have enter and exit methods, in particular, in google protobuf, adding
    entries to a repeated field of messages. syntax:
       with withpb(msg.some_repeated_field().add()) as newfield:
          newfield.blah = blah"""
    f: object

    def __init__(self, f: object):
        self.f = f

    def __enter__(self):
        return self.f

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass


class WebsocketProtobufServer:
    ok: bool
    _debug_text: bool
    _debug_binary: bool
    _recv_msg: pbmsg.Message
    _recv_q: queue.Queue
    _port: int
    _svr: wsserver.Server
    _conns: dict[int, wsserver.ServerConnection]

    class QmsgType(Enum):
        CONNECT = 1
        DISCO = 2
        MSG = 3
        EXIT = 4

    CONNECT = QmsgType.CONNECT
    DISCO = QmsgType.DISCO
    MSG = QmsgType.MSG
    EXIT = QmsgType.EXIT

    class _Qmsg:
        def __init__(self, conn_id: int,
                     t: WebsocketProtobufServer.QmsgType,
                     msg: Union[pbmsg.Message, None] = None):
            self.conn_id = conn_id
            self.t = t
            self.msg = msg

    def __init__(self, port: int,
                 recv_msg_factory: Callable[[], pbmsg.Message],
                 debug_text: bool = False,
                 debug_binary: bool = False):
        self._port = port
        self._recv_msg_factory = recv_msg_factory
        self._debug_text = debug_text
        self._debug_binary = debug_binary
        self._recv_q = queue.Queue()
        self._conns = {}
        self._svr = wsserver.serve(handler=self._conn_handler,
                                   host="",
                                   port=self._port)
        self._svr_thread = threading.Thread(target=self._svr.serve_forever,
                                            daemon=True)
        self._svr_thread.start()

    def _conn_handler(self, conn: wsserver.ServerConnection):
        recv_msg: pbmsg.Message = self._recv_msg_factory()
        while True:
            conn_id = random.randrange(1, 1000000)
            if conn_id not in self._conns:
                break
        self._conns[conn_id] = conn
        self._recv_q.put(
            WebsocketProtobufServer._Qmsg(
                conn_id,
                WebsocketProtobufServer.QmsgType.CONNECT))
        try:
            for msg in conn:
                if self._debug_binary:
                    print(f'got {len(msg)} bytes:')
                    print(' '.join('{:02x}'.format(c) for c in msg))
                recv_msg.Clear()
                try:
                    recv_msg.ParseFromString(msg)
                    if self._debug_text:
                        print(f'decoded recvd message: {recv_msg}')
                    self._recv_q.put(
                        WebsocketProtobufServer._Qmsg(
                            conn_id,
                            WebsocketProtobufServer.QmsgType.MSG,
                            recv_msg))
                except Exception as e:
                    print(f'while parsing: {e}')
        except Exception as e:
            print(f'got exception: {e}')
        self._recv_q.put(
            WebsocketProtobufServer._Qmsg(
                conn_id,
                WebsocketProtobufServer.QmsgType.DISCO))
        del self._conns[conn_id]

    def get_message(self) -> (int,
                              WebsocketProtobufServer.QmsgType,
                              Union[pbmsg.Message, str]):
        qm: WebsocketProtobufServer._Qmsg
        qm = self._recv_q.get()
        if qm:
            return qm.conn_id, qm.t, qm.msg
        return 0, self.EXIT, ''

    def send_message(self, conn_id: int, msg: pbmsg.Message,
                     all_but: int = -1) -> (bool, str):
        if conn_id == 0:
            clist = [c for i, c in self._conns.items() if i != all_but]
        else:
            conn = self._conns.get(conn_id, None)
            if not conn:
                return False, "conn_id not valid"
            clist = [conn]
        body = msg.SerializeToString()
        if self._debug_text:
            print(f'sending message: {msg}')
        if self._debug_binary:
            print(f'sending msg encoded as:')
            print(' '.join('{:02x}'.format(c) for c in body))
        try:
            for conn in clist:
                conn.send(body)
        except Exception as e:
            err = f'failed to send message: {e}'
            print(err)
            return False, err
        return True, "sent"


class WebsocketProtobufClient:
    """
    implement protobuf encoding and decoding over websocket (RFC6455).
    """
    CONNECT_TIMEOUT = 5.0
    CLOSE_TIMEOUT = 1.0
    RECV_TIMEOUT = 1.0
    ok: bool
    _debug_text: bool
    _debug_binary: bool
    _recv_msg: pbmsg.Message
    _close: bool

    def __init__(self, addr: str, port: int, path: str,
                 recv_msg: pbmsg.Message,
                 connect_timeout: float = CONNECT_TIMEOUT,
                 debug_text: bool = False,
                 debug_binary: bool = False):
        """
        construct a WebsocketProtobuf client.
        @param addr: an IP address as a dotted quad string 'a.b.c.d'
        @param port: TCP port number of the websocket server.
        @param path: the path component of the URL.
        @param recv_msg: construct a Message object to be used for receiving.
        @param connect_timeout: give up after this many seconds.
        @param debug_text: print all protobuf msg contents
        @param debug_binary: print hex bytes for all websocket msgs
        """
        self.ok = False
        self._debug_text = debug_text
        self._debug_binary = debug_binary
        self._recv_msg = recv_msg
        self._close = False
        uri = f'ws://{addr}:{port}{path}'
        try:
            self._sock = wsclient.connect(
                uri=uri,
                open_timeout=connect_timeout,
                close_timeout=self.CLOSE_TIMEOUT)
            self.ok = True
        except Exception as e:
            print(f'failed to connect to {addr}:{port}: {e}')

    def close(self):
        """
        close this web socket. if there is a currently blocking
        get_message, it will return False very quickly after this.
        @return: None
        """
        self._close = True
        self._sock.close()

    def send_message(self, msg: pbmsg.Message) -> bool:
        """
        encode and send a protobuf message on the websocket in BINARY mode.
        @param msg: the Message to send
        @return:  True if success, False if the message could not be sent.
          If this function fails, the socket should be closed.
        """
        body = msg.SerializeToString()
        if self._debug_text:
            print(f'sending message: {msg}')
        if self._debug_binary:
            print(f'sending msg encoded as:')
            print(' '.join('{:02x}'.format(c) for c in body))
        try:
            self._sock.send(body)
        except Exception as e:
            print(f'failed to send message: {e}')
            return False
        return True

    def get_message(self) -> (bool, Union[pbmsg.Message, str]):
        """
        receive and decode the next protobuf message. NOTE it returns
        the same object every call, so be sure to consume the contents
        before the next call. this function only supports receiving
        BINARY websocket messages (not text).
        this function blocks; but if you call close() on this class, this
        function will return (False, 'closed') almost immediately.
        @return: (True, Message) or (False, error_string)
        """
        while True:
            try:
                msg = self._sock.recv(timeout=self.RECV_TIMEOUT)
                break
            except TimeoutError:
                if self._close:
                    return False, f'closed'
            except Exception as e:
                if self._close:
                    return False, f'closed'
                return False, f'while receiving: {e}'
        if self._debug_binary:
            print(f'got {len(msg)} bytes:')
            print(' '.join('{:02x}'.format(c) for c in msg))
        self._recv_msg.Clear()
        try:
            self._recv_msg.ParseFromString(msg)
        except Exception as e:
            return False, f'while parsing: {e}'
        if self._debug_text:
            print(f'decoded recvd message: {self._recv_msg}')
        return True, self._recv_msg
