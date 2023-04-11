
class MyFSConfig:
    DEFAULT_SERVER_PORT = 21005
    FILE_BUFFER_SIZE = 16384
    MAX_DIR_SIZE = 32000
    MAX_MESSAGE_LEN = 65000
    root_dir: str
    username: str
    password: str
    addr: str
    server_port: int
    debug_text: bool
    debug_binary: bool
