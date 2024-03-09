
import threading
import queue
import sys


class ThreadedPrinter:
    """
    to prevent multithreaded printouts from interveaving in an ugly
    fashion, use this class to serialize printouts at a line-by-line level.
    use thusly:
      from ThreadedPrinter import tprint, tprinter
      tprint(f'some string with {some_value}')
    when the main process is exiting, be sure to call this:
      tprinter.stop()
    this will ensure the thread is stopped and the last prints make it out.
    the only difference between tprint() and print() is that tprint() only
    handles the first arg, where print() can do many. just use f'' and
    concatenate strings if you need to, and you'll be fine.
    """
    _t: threading.Thread
    _q: queue.Queue

    def _print(self):
        go = True
        while go:
            msg = self._q.get()
            if isinstance(msg, str):
                if msg == 'stop':
                    go = False
                else:
                    print(msg)
                    sys.stdout.flush()

    def __init__(self):
        self._q = queue.Queue()
        self._t = threading.Thread(target=self._print, daemon=True)
        self._t.start()

    def __del__(self):
        self.stop()

    def stop(self):
        self._q.put('stop')
        self._t.join()

    def print(self, msg: str):
        self._q.put(msg)


tprinter: ThreadedPrinter = ThreadedPrinter()
tprint = tprinter.print
