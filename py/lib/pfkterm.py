
import os
import re
import time
import sys
import select
import tty
import termios
import curses


class TermControl:
    def __init__(self, use_curses: bool, rows: int = 0, cols: int = 0):
        self.infd = os.fdopen(0, buffering=False, mode='rb')
        self.old_settings = termios.tcgetattr(0)
        # tty.setraw(self.infd)
        tty.setcbreak(self.infd)
        self.old_rows, self.old_cols = self.get_window_size()
        if rows and cols:
            self.set_window_size(rows, cols)
        if use_curses:
            self.scr = curses.initscr()
            curses.noecho()
            curses.cbreak()
            self.scr.keypad(True)
        else:
            self.scr = None

    def __del__(self):
        if self.scr:
            self.scr.keypad(False)
            del self.scr
            curses.nocbreak()
            curses.echo()
            curses.endwin()
        self.set_window_size(self.old_rows, self.old_cols)
        termios.tcsetattr(0, termios.TCSADRAIN, self.old_settings)

    def _read_from_term(self) -> (int, int):
        """returns string read from input"""
        go = True
        buf = bytes()
        while go:
            rd, wr, ex = select.select([self.infd], [], [], 0.15)
            if len(rd) > 0:
                buf += self.infd.read(1)
            else:
                go = False
        return buf

    def get_window_size(self):
        """send the 'report window size' command and parse the response"""
        print('\033[18;t', end='')
        sys.stdout.flush()
        buf = self._read_from_term()
        # print(f'got {len(buf)}:{buf}')
        m = re.search('\x1b\\[8;([0-9]+);([0-9]+)t'.encode(), buf)
        if not m:
            return None, None
        return int(m.group(1)), int(m.group(2))

    @staticmethod
    def set_window_size(rows: int, cols: int):
        """resize window, home cursor, erase to end of screen"""
        print(f'\033[8;{rows};{cols}t\033[H\033[J')
        time.sleep(0.1)
