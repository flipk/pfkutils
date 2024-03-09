import time
from typing import Callable
import os
import threading
import signal
import _prctl


class Reaper:
    """
    this class uses the linux 'SUBREAPER' function to collect zombies
    and orphaned child processes, and 'reap' them (using 'waitpid').
    this prevents the buildup of zombie processes.
    """
    _run: bool

    def __init__(self):
        self._run = True
        t = threading.Thread(target=self._waiter_thread, daemon=True)
        t.start()
        _prctl.prctl(_prctl.PR_SET_CHILD_SUBREAPER, 1)

    def _waiter_thread(self):
        while self._run:
            try:
                res = os.waitpid(-1, 0)
                print(f'waitpid returned {res}')
            except ChildProcessError:
                # when we have NO child processes, os.waitpid
                # returns an error. but we still need to collect future
                # children, so sleep for a bit and just keep going
                # around. we'll get them eventually.
                time.sleep(0.1)

    @staticmethod
    def cleanup_children(match: Callable[[str], bool],
                         term2killdelay: float = 5.0):
        """
        first, send all matching processes SIGTERM. give them time to
        die; if there are some that still exists, send SIGKILL. give
        them a chance to die, and repeat a couple more times.
        :param match:   a function which return True for matching processes
        :param term2killdelay:
        :return:
        """
        def _do_walk(sig) -> int:
            """
            iterate over all processes looking for matches; for each match,
            sends the specified signal.
            :param sig: the signal (SIGTERM or SIGKILL) to send.
            :return:  the number of matching processes that were sent the signal
            """
            count = 0
            de: os.DirEntry
            for de in os.scandir('/proc'):
                try:
                    pid = int(de.name)
                    with open(f'/proc/{de.name}/comm', mode='r') as fd:
                        cmd = fd.read()
                    cmd = cmd.strip()
                    m = match(cmd)
                    if m:
                        print(f'CLEANUP: killing pid {pid} "{cmd}" with sig {sig}')
                        os.kill(pid, sig)
                        count += 1
                except ValueError:
                    pass
                except PermissionError:
                    pass
            return count

        if _do_walk(signal.SIGTERM) > 0:
            time.sleep(term2killdelay)
            if _do_walk(signal.SIGKILL) > 0:
                time.sleep(term2killdelay)
                if _do_walk(signal.SIGKILL) > 0:
                    time.sleep(term2killdelay)
                    _do_walk(signal.SIGKILL)
