#!/usr/bin/env python3

import sys
import threading
from WebsocketProtobuf import WebsocketProtobufClient as wsclient
import websocket_test_pb2 as wststpb


class RdrInfo:
    conn: wsclient | None
    conn_id: int
    username: str
    msg: wststpb.ClientMsg
    done: bool

    def __init__(self, username: str):
        self.conn = None
        self.conn_id = -1
        self.username = username
        self.done = False
        self.msg = wststpb.ClientMsg()


def reader(info: RdrInfo):
    try:
        while not info.done:
            info.msg.Clear()
            info.msg.type = wststpb.MESSAGE_TYPE_DATA
            info.msg.data.data = input()
            info.msg.conn_id = info.conn_id
            if len(info.msg.data.data) > 0:
                info.conn.send_message(info.msg)
    except EOFError:
        info.conn.close()
        info.done = True


def main(argv) -> int:
    if len(argv) != 3:
        print('usage: client <ipaddr> <username>')
        return 1
    addr = argv[1]
    username = argv[2]
    port = 12345
    path = '/websocket/fancy'
    msg = wststpb.ClientMsg()
    c = wsclient(addr, port, path, msg)
    if not c.ok:
        return 1
    msg.Clear()
    msg.type = wststpb.MessageType.MESSAGE_TYPE_HELLO
    msg.username = username
    c.send_message(msg)
    rdrinfo = RdrInfo(username)
    rdrinfo.conn = c
    rdr_thread = threading.Thread(target=reader,
                                  args=[rdrinfo],
                                  daemon=True)
    rdr_thread.start()
    while True:
        msg: wststpb.ClientMsg
        ok, msg = c.get_message()
        if ok:
            if msg.type == wststpb.MESSAGE_TYPE_HELLO:
                print(f'connected: assigned conn_id {msg.conn_id}')
                rdrinfo.conn_id = msg.conn_id
            elif msg.type == wststpb.MESSAGE_TYPE_BYE:
                print(f'server says BYE')
                c.send_message(msg)
                break
            elif msg.type == wststpb.MESSAGE_TYPE_DATA:
                print(f'{msg.conn_id}: {msg.username}: {msg.data.data}')
        else:
            if msg != 'closed':
                print(f'error while receiving: {msg}')
            break
    print('disconnected!')
    c.close()
    del c
    return 0


if __name__ == '__main__':
    try:
        main(sys.argv)
    except KeyboardInterrupt:
        print('\nKeyboard Interrupt')
    exit(0)
