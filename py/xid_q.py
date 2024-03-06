import queue
import random


class xid_q:
    """create a random transaction ID and register a queue with the
    interface class that can process Status messages with that xid.
    if you use the 'with' syntax (python context manager) it will
    automatically register and deregister the xid.
    class attributes:
        xid: the random transaction id allocated
        q: the Queue on which Status responses will arrive"""
    xid: int
    q: queue.Queue

    def __init__(self, qi):
        self.xid = random.randint(1, 999999999)
        self.q = queue.Queue()
        self._qi = qi

    def __enter__(self):
        self._qi.register_xid(self.xid, self.q)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._qi.unregister_xid(self.xid)
