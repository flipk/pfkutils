
import time


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
    def total(self) -> int:
        return self._total

    def peg(self):
        self._iter_count += 1
        self._total += 1

    def format(self) -> str:
        # ret = self._name + ":%4d/%6d" % (self._iter_count, self._total)
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
    a container for a set of stats. the format method automatically
    iterates over every registered stat and calls format() on each
    stat in turn.
    """
    _ss: list

    def __init__(self):
        self._ss = []

    # i wanted to hint "stat" as Stat but python doesn't have
    # forward declarations (to be clear, python doesn't need it
    # but i don't know how to clear a warning from PyCharm)
    def stat(self, name: str, timestat: bool = False) -> Stat:
        s = Stat(name, timestat)
        self._ss.append(s)
        return s

    def format(self) -> str:
        ret = ""
        first = True
        for s in self._ss:
            if not first:
                ret += " "
            first = False
            ret += s.format()
        return ret
