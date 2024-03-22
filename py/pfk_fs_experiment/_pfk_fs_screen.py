
import _pfk_fs
import curses


class Screen:
    fs_list: list[_pfk_fs.Fs]
    widths: _pfk_fs.Widths
    cols: _pfk_fs.Widths
    selected: int
    passwords: dict
    rows: dict
    err: curses.error | None

    def __init__(self, fs_list: list[_pfk_fs.Fs]):
        self.fs_list = fs_list
        self.widths = _pfk_fs.Widths()
        for f in fs_list:
            self.widths.max(f.widths)
        self.cols = _pfk_fs.Widths()
        self.cols.calc_cols(self.widths)
        self.selected = 100
        self.passwords = {}
        self.rows = {}
        self.err = None

        self.scr = curses.initscr()
        curses.noecho()
        curses.cbreak()
        self.scr.keypad(True)

    def __del__(self):
        self.scr.keypad(False)
        del self.scr
        curses.nocbreak()
        curses.echo()
        curses.endwin()
        if self.err:
            print(f'ERROR: {self.err}')

    def _draw(self):
        scr = self.scr
        widths = self.widths
        cols = self.cols
        scr.addstr(0, cols.num, '##', curses.A_UNDERLINE)
        scr.addstr(0, cols.name, 'name', curses.A_UNDERLINE)
        if widths.depends > 0:
            scr.addstr(0, cols.depends, 'depends', curses.A_UNDERLINE)
        if widths.luks > 0:
            scr.addstr(0, cols.luks, 'luks', curses.A_UNDERLINE)
        if widths.passgrp > 0:
            scr.addstr(0, cols.passgrp, 'p', curses.A_UNDERLINE)
        if widths.open > 0:
            scr.addstr(0, cols.open, 'open', curses.A_UNDERLINE)
        # ✓✔√
        if widths.checked > 0:
            scr.addstr(0, cols.checked, '✓ed', curses.A_UNDERLINE)
        if widths.nfs > 0:
            scr.addstr(0, cols.nfs, 'nfs', curses.A_UNDERLINE)
        scr.addstr(0, cols.mntpt, 'mnt point', curses.A_UNDERLINE)
        scr.addstr(0, cols.mounted, 'mounted', curses.A_UNDERLINE)
        rownum = 1
        selected_fs = None
        for fs in self.fs_list:
            if rownum == self.selected:
                selected_fs = fs
                scr.attron(curses.A_REVERSE)
            scr.addstr(rownum, cols.num, f'{rownum:2d}')
            scr.addstr(rownum, cols.name, f'{fs.name}')
            if fs.depends:
                scr.addstr(rownum, cols.depends, f'{fs.depends}')
            if fs.luks:
                scr.addstr(rownum, cols.luks, f'{fs.luks}')
            else:
                scr.addstr(rownum, cols.luks, 'n/a')
            if fs.pass_group:
                scr.addstr(rownum, cols.passgrp, f'{fs.pass_group}')
            if fs.open:
                scr.addstr(rownum, cols.open, 'yes')
            else:
                scr.addstr(rownum, cols.open, 'no ')
            if widths.checked > 0:
                if fs.checked:
                    scr.addstr(rownum, cols.checked, 'yes')
                else:
                    scr.addstr(rownum, cols.checked, 'no ')
            if widths.nfs > 0:
                scr.addstr(rownum, cols.nfs, f'{fs.nfs}')
            scr.addstr(rownum, cols.mntpt, f'{fs.mntpt}')
            if fs.mounted:
                scr.addstr(rownum, cols.mounted, 'yes')
            else:
                scr.addstr(rownum, cols.mounted, 'no ')
            scr.attroff(curses.A_REVERSE)
            rownum += 1
        rownum += 1
        selected_passnum = None
        for p in self.passwords.keys():
            if self.selected == 100 + p:
                selected_passnum = p
                scr.attron(curses.A_REVERSE)
            pw = self.passwords[p]
            loaded = pw if len(pw) > 0 else "no "
            scr.addstr(rownum, 0, f'password {p} loaded: {loaded}')
            scr.attroff(curses.A_REVERSE)
            rownum += 1
        rownum += 1
        self.rows['menuline'] = rownum
        scr.move(rownum, 0)
        if selected_fs:
            if not selected_fs.open:
                passnum = selected_fs.pass_group
                if passnum:
                    password = self.passwords[passnum]
                    if len(password) == 0:
                        scr.addstr(f'(pass {passnum} is not loaded) ')
                    else:
                        scr.addstr('<o>pen-luks ')
            else:
                if widths.checked > 0:
                    if not selected_fs.checked:
                        scr.addstr('<f>sck ')
                if not selected_fs.mounted:
                    if selected_fs.luks:
                        scr.addstr('<c>lose-luks ')
                    scr.addstr('<m>ount ')
                else:
                    scr.addstr('<u>mount ')
        elif selected_passnum:
            scr.addstr('<enter> load password ')
        scr.addstr(' <q>uit')
        scr.clrtoeol()
        rownum += 1
        self.rows['cursorline'] = rownum
        scr.move(rownum, 0)

    def draw(self):
        self.err = None
        try:
            self._draw()
        except curses.error as e:
            self.err = e

