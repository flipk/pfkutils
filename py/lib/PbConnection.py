from typing import Callable, Union
import os
import threading
import socket
# noinspection PyPackageRequirements
from google.protobuf import message


class PbConnection:
    """
    Generic class for handling google protobuf messages over a socket
    pair.  Useful for TCP (using socket.socket) or raw file
    descriptors with FIFOs.  Can optionally run a thread which does
    receive, invoking callbacks for incoming messages, or
    alternatively provide a polling method for getting incoming
    messages.
    """
    _sock_in: socket.socket | int
    _sock_out: socket.socket | int
    _same_sock: bool  # is the in and out the same sock?
    _ok: bool
    _recv_msg: message
    _debug_text: bool
    _debug_binary: bool
    _recvdata: bytes
    _thread: threading.Thread | None
    _callback_closed: Union[Callable[[str], None], None]
    _callback_msg: Callable[[message], None]
    MAX_MESSAGE_LEN = 65000

    def __init__(self,
                 sock_in: socket.socket | int,
                 sock_out: socket.socket | int,
                 addr,
                 recv_msg: message,
                 callback_msg: Union[Callable[[message], None], None],
                 callback_closed: Union[Callable[[], None], None],
                 debug_text: bool,
                 debug_binary: bool):
        """
        Construct PbConnection class. If doing a TCP socket, pass the
        same 'socket.socket' for both sock_in and sock_out (it will
        handle both being the same). If doing a FIFO (linux named
        pipe) pass in two different raw file descriptors (ints from
        os.open).  If you want this class to create a thread to do the
        receiving, pass callback functions for callback_msg and
        _closed (and do not call get_message).  IF you dont want the
        helper reader thread, pass None for those two args, but then
        you must call get_message.  You must also provide an empty
        object for the message type you expect to receive, in the
        recv_msg arg.
        :param sock_in: socket.socket, or an int (from os.open)
        :param sock_out: socket.socket or an int
        :param addr: a tuple uniqely identifying the connection
        :param recv_msg: an empty object of the type you
            expect to receive
        :param callback_msg: a Callable that will receive inbound msgs.
        :param callback_closed: a Callable that will be called
            if the conn closes.
        :param debug_text:  display protobuf as text debug?
        :param debug_binary:  display encoded binary protobuf?
        """
        try:
            self._debug_text = debug_text
            self._debug_binary = debug_binary
            self._sock_in = sock_in
            self._sock_out = sock_out
            self._same_sock = sock_in == sock_out
            self._addr = addr
            self._recv_msg = recv_msg
            self._callback_msg = callback_msg
            self._callback_closed = callback_closed
            self._recvdata = bytes()
            if self._callback_msg:
                self._thread = threading.Thread(
                    target=self._handle_connection,
                    args=[], daemon=True)
                self._thread.start()
            else:
                self._thread = None
            self._ok = True
        except Exception as e:
            print(f'Unable to init PbConnection: {e}')
            self._ok = False
            raise

    @property
    def ok(self) -> bool:
        return self._ok

    def send_message(self, msg: message):
        """
        send an outbound messages on the sock_out.
        :param msg:   the protobuf message object to send.
        :return:   Nothing
        """
        body = msg.SerializeToString()
        length = len(body)  # m.ByteSize()
        encoded = length.to_bytes(2, byteorder='big') + body

        if self._debug_text:
            print(f'sending message: {msg}')
        if self._debug_binary:
            print('encoded as:')
            print(" ".join("{:02x}".format(c) for c in encoded))
        if isinstance(self._sock_out, socket.socket):
            self._sock_out.send(encoded)
        else:
            os.write(self._sock_out, encoded)

    def _decode_msg(self, msg: bytes) -> bool:
        if self._debug_binary:
            print(f'got {len(msg)} bytes: ')
            print(' '.join('{:02x}'.format(c) for c in msg))
        try:
            self._recv_msg.Clear()
            self._recv_msg.ParseFromString(msg)
        except Exception as e:
            print(f'while parsing message: {e}')
            return False
        if self._debug_text:
            print(f'decoded message: {self._recv_msg}')
        return True

    def get_message(self, internal=False) -> (bool, Union[message, str]):
        """
        get the next message from the sock_in.
        DO NOT CALL THIS if you specified callback_msg in the constructor.
        :return:  success or fail; a message if success, or a reason string if fail.
        """
        if not internal and self._callback_msg:
            return False, 'ERR: DO NOT CALL PbConnection.get_message WITH A CALLBACK SET'
        while True:
            if len(self._recvdata) > 2:
                length = int.from_bytes(self._recvdata[0:2], byteorder='big')
                if length > self.MAX_MESSAGE_LEN:
                    # bogusly sent message, or maybe garbage.
                    return False, f'bogus message length {length}'
                if len(self._recvdata) >= (length + 2):
                    if self._decode_msg(self._recvdata[2:length + 2]):
                        if len(self._recvdata) == (length+2):
                            # we kept appending to this thing, who knows how big it
                            # got, and whether the slicing in the 'else' below actually
                            # frees any memory. make sure it gets freed whenever the link
                            # actually goes idle.
                            del self._recvdata
                            self._recvdata = bytes()
                        else:
                            self._recvdata = self._recvdata[length + 2:]
                        return True, self._recv_msg
                    else:
                        return False, f'failed to decode message! {length}'
            if isinstance(self._sock_in, socket.socket):
                buf = self._sock_in.recv(32768)
            else:
                buf = os.read(self._sock_in, 32768)
            if len(buf) == 0:
                if isinstance(self._sock_in, socket.socket):
                    self._sock_in.close()
                    if not self._same_sock:
                        self._sock_out.close()
                else:
                    os.close(self._sock_in)
                    if not self._same_sock:
                        os.close(self._sock_out)
                return False, f'connection closed'
            self._recvdata += buf

    def _handle_connection(self):
        while True:
            success, msg_or_err = self.get_message(internal=True)
            if not success:
                break
            self._callback_msg(msg_or_err)
        if self._callback_closed:
            self._callback_closed(msg_or_err)
