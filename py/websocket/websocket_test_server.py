#!/usr/bin/env python3

from WebsocketProtobuf import WebsocketProtobufServer as wssvr
from google.protobuf import message as pbmsg
import websocket_test_pb2 as wststpb


def rcv_msg_factory() -> pbmsg.Message:
    return wststpb.ClientMsg()


class SvrConn:
    def __init__(self, conn_id: int):
        self.conn_id = conn_id
        self.username = '<unassigned>'


def main() -> int:
    port = 12345
    s = wssvr(port, rcv_msg_factory)
    if not s.ok:
        return 1
    tmsg = wststpb.ClientMsg()
    conns: dict[int, SvrConn] = {}
    while True:
        rmsg: wststpb.ClientMsg
        conn_id, t, rmsg = s.get_message()
        print(f'got msg conn_id {conn_id} type {t}: {rmsg}')
        if t == wssvr.CONNECT:
            sc = SvrConn(conn_id)
            conns[conn_id] = sc
            # assign conn_id to the client.
            tmsg.Clear()
            tmsg.type = wststpb.MESSAGE_TYPE_HELLO
            tmsg.conn_id = conn_id
            s.send_message(conn_id, tmsg)
        elif t == wssvr.MSG:
            if rmsg.type == wststpb.MESSAGE_TYPE_HELLO:
                sc = conns.get(conn_id, None)
                if sc:
                    sc.username = rmsg.username
                    # tell the other users so-and-so has arrived.
                    tmsg.Clear()
                    tmsg.conn_id = conn_id
                    tmsg.username = sc.username
                    tmsg.type = wststpb.MESSAGE_TYPE_DATA
                    tmsg.data.data = f"user {sc.username} has joined"
                    s.send_message(0, tmsg, all_except=conn_id)
                    # tell this user all the other users signed in
                    for sc in conns.values():
                        if sc.conn_id == conn_id:
                            continue
                        tmsg.Clear()
                        tmsg.conn_id = sc.conn_id
                        tmsg.username = sc.username
                        tmsg.type = wststpb.MESSAGE_TYPE_DATA
                        tmsg.data.data = f'user is here'
                        s.send_message(conn_id, tmsg)
            if rmsg.type == wststpb.MESSAGE_TYPE_DATA:
                sc = conns.get(conn_id, None)
                if sc:
                    rmsg.username = sc.username
                    s.send_message(0, rmsg, all_except=conn_id)
                    if rmsg.data.data == 'server quit':
                        s.shutdown()
        elif t == wssvr.DISCO:
            sc = conns.get(conn_id, None)
            if sc:
                tmsg.Clear()
                tmsg.type = wststpb.MESSAGE_TYPE_DATA
                tmsg.conn_id = conn_id
                tmsg.username = sc.username
                tmsg.data.data = f"user {sc.username} has left"
                del conns[conn_id]
            s.send_message(0, tmsg)
        elif t == wssvr.EXIT:
            break
    return 0


if __name__ == '__main__':
    r = 0
    try:
        r = main()
    except KeyboardInterrupt:
        print('\nKeyboard Interrupt')
    exit(r)
