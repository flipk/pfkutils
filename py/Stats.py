
import queue
import time
import threading
from typing import Callable


class Stat:
    """
    a stat. contains a total count, a per-display iteration count,
    and a stat name. the format method constructs a string containing
    NAME:ITER/TOTAL and then resets the iter count back to zero for
    the next print iteration. each stat object automatically registers
    itself with a Stats object so every Stats.format() automatically
    formats all the stats, and resets the iter counts too.
    """
    _total: int
    _iter_count: int
    _name: str
    _timestat: bool
    _start: float

    def __init__(self, name: str, timestat: bool):
        self._iter_count = 0
        self._total = 0
        self._name = name
        self._timestat = timestat
        self._start = time.monotonic()

    @property
    def name(self) -> str:
        return self._name

    @property
    def total(self) -> int:
        return self._total

    def peg(self, count=1):
        self._iter_count += count
        self._total += count

    def format(self) -> str:
        ret = self._name + ":" + str(self._iter_count) + "/" + str(self._total)
        if self._timestat:
            now = time.monotonic()
            diff = now - self._start
            self._start = now
            rate = self._iter_count / diff
            ret += "/" + "%.0f" % rate
        self._iter_count = 0
        return ret


class Stats:
    """
    a container for a set of stats.
    """
    _ss: dict[str, Stat]
    _default_timestat: bool
    _print_interval: float
    _print_func: Callable[[str], None]
    _thread_shutdown_q: queue.Queue
    _thread_running: bool

    def __init__(self,
                 default_timestat: bool = False,
                 print_interval: float = 1.0,
                 print_func: Callable[[str], None] = print
                 ):
        self._ss = {}
        self._default_timestat = default_timestat
        self._print_interval = print_interval
        self._print_func = print_func
        self._thread_shutdown_q = queue.Queue()
        self._stats_thread = threading.Thread(target=self._stats_printer, daemon=True)
        self._stats_thread.start()
        self._thread_running = True

    def __del__(self):
        self.stop()

    def get(self, name: str) -> Stat:
        if name in self._ss:
            return self._ss[name]
        s = Stat(name, self._default_timestat)
        self._ss[name] = s
        return s

    def stop(self):
        if self._thread_running:
            self._thread_shutdown_q.put('stop')
            self._stats_thread.join()
        self._thread_running = False

    def _format(self) -> str:
        ret = ""
        for s in self._ss.values():
            ret += s.format() + "  "
        return ret

    def _stats_printer(self):
        go = True
        while go:
            try:
                msg = self._thread_shutdown_q.get(timeout=self._print_interval)
                if msg == 'stop':
                    go = False
            except queue.Empty:
                pass
            self._print_func(self._format())
