
from typing import TextIO
import json
import collections.abc


class JsonReader:
    """
    A class for reading a JSON file of {}\n{}\n{}\n assuming
    there is one JSON record on each line.
    Args: filename
    """
    _handle: TextIO
    _ok: bool
    _lines: int

    def __init__(self, filename):
        self._lines = 0
        try:
            self._handle = open(filename, 'r', encoding='utf-8')
            self._ok = True
        except BaseException as e:
            print(e)
            self._ok = False

    @property
    def ok(self) -> bool:
        return self._ok

    @property
    def lines(self) -> int:
        return self._lines

    def records(self) -> collections.abc.Iterable:
        """
        Generator: Reads one line and returns (json dict).
        """
        while self._ok:
            line = ''
            try:
                line = self._handle.readline()
                if len(line) == 0:
                    # end of file!
                    self._ok = False
                    break
                rec = json.loads(line)
                self._lines += 1
                yield rec
            except BaseException as e:
                print("\n" "on line %d line = '%s'" % (self._lines, line))
                print(e)
                self._ok = False
