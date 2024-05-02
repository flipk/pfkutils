
import subprocess
import fcntl
import os
import sys
import select


class MyPipe (subprocess.Popen):

    def __init__(self, cmd: list[str]):
        super().__init__(cmd,
                         close_fds=True,
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT,
                         encoding='utf8')

    # TODO consider using os.read on the stdout fileno;
    #      do you need NONBLOCK then?
    def reader(self, copy_stdout: bool = True) -> str:
        ret = ''
        fd = self.stdout
        fileno = fd.fileno()
        flags = fcntl.fcntl(fileno, fcntl.F_GETFL)
        fcntl.fcntl(fileno, fcntl.F_SETFL, flags | os.O_NONBLOCK)
        while True:
            # fd is nonblocking, so the only way to block on it is with select.
            # nonblock is the only way to get short reads (???)
            r, w, x = select.select([fd], [], [], .2)
            if len(r) > 0:
                buf = fd.read()
                if buf:
                    ret += buf
                    if copy_stdout:
                        print(buf, end='')
                    sys.stdout.flush()
                else:
                    break
        self.wait()
        return ret
