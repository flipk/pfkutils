#!/usr/bin/env python3

from WebsocketProtobuf import WebsocketProtobufServer as wssvr
from google.protobuf import message as pbmsg
import websocket_test_pb2 as wststpb


def rcv_msg_factory() -> pbmsg.Message:
    return wststpb.ClientMsg()


def main() -> int:
    port = 12345
    s = wssvr(port, rcv_msg_factory)
    tmsg = wststpb.ClientMsg()
    while True:
        rmsg: wststpb.ClientMsg
        conn_id, t, rmsg = s.get_message()
        print(f'got msg conn_id {conn_id} type {t}: {rmsg}')
        if t == wssvr.CONNECT:
            tmsg.Clear()
            tmsg.type = wststpb.MESSAGE_TYPE_HELLO
            tmsg.conn_id = conn_id
            s.send_message(conn_id, tmsg)
            tmsg.Clear()
            tmsg.conn_id = conn_id
            tmsg.type = wststpb.MESSAGE_TYPE_DATA
            tmsg.data.data = f"new conn_id {conn_id} has joined"
            s.send_message(0, tmsg, all_but=conn_id)
        elif t == wssvr.MSG:
            if rmsg.type == wststpb.MESSAGE_TYPE_DATA:
                s.send_message(0, rmsg,
                               all_but=conn_id)
        elif t == wssvr.DISCO:
            tmsg.Clear()
            tmsg.type = wststpb.MESSAGE_TYPE_DATA
            tmsg.conn_id = conn_id
            tmsg.data.data = f"conn_id {conn_id} has left"
            s.send_message(0, tmsg)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print('\nKeyboard Interrupt')
    exit(0)
