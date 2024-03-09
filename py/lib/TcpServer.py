from typing import Callable
import socket


class TcpServer:
    """
    Simple TCP server opens a listening socket and waits for a connection;
    calls a user supplied callback when a new connection is accepted.
    """
    _sock: socket.socket
    _port_num: int
    _new_conn_callback: Callable[[socket.socket, object], None]
    _ok: bool

    def __init__(self, port_number: int,
                 new_conn_callback: Callable[[socket.socket, object], None]):
        """
        Initialize the TcpServer; creates the socket. Check the 'ok' property
        after this to see if this initialized ok or not. Then call 'serve'.
        :param port_number:  The TCP port number to listen for connections on.
        :param new_conn_callback:  A callback function called
           when new connection accepted.
        """
        self._ok = False
        try:
            self._sock = socket.socket()
            self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
            self._sock.bind(('', port_number))
            self._port_num = port_number
            self._new_conn_callback = new_conn_callback
            self._ok = True
        except Exception as e:
            print(f'TcpServer initialization failed: {e}')

    @property
    def ok(self) -> bool:
        return self._ok

    def serve(self):
        """
        Implements the server, waiting for connections and invoking the callback
        for each new connection accepted.
        :return:   Does not return.
        """
        self._sock.listen(1)
        print(f'TcpServer ready to handle new connections on port {self._port_num}')
        while True:
            new_s, addr = self._sock.accept()
            self._new_conn_callback(new_s, addr)
