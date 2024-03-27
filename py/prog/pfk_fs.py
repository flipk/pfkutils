#!/usr/bin/env python3

import os
import sys

sys.path.append(f'{os.environ["HOME"]}/proj/pfkutils/py/lib')

import curses
import select
import subprocess
import re
import tempfile
import _pfk_fs
import pfk_fs_config
import time
import json
import pfkterm


class Rows:
    menuline: int
    cursorline: int
    statusline: int

    def __init__(self):
        self.menuline = 0
        self.cursorline = 0
        self.statusline = 0


rows = Rows()
widths = _pfk_fs.Widths()
cols = _pfk_fs.Widths()
passwords = {}
mounts = {}
ticklers = {}
nfs_server_running = False


class PfkFsErrorStatus(Exception):
    errstr: str

    def __init__(self, errstr: str):
        self.errstr = errstr + '\n\nIs tickler running?\n'

    def __str__(self):
        return self.errstr


def measure_widths():
    global widths
    global cols
    for f in pfk_fs_config.fs_list:
        widths.max(f.widths)
    cols.calc_cols(widths)


def run_command(cmd: list[str], do_split: bool = True):
    try:
        cp = subprocess.run(cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            timeout=30)
    except subprocess.TimeoutExpired:
        print(f'ERROR: subprocess {cmd} timeout')
        time.sleep(5)
        exit(1)
    if do_split:
        stdoutlines = cp.stdout.decode().split('\n')
    else:
        stdoutlines = cp.stdout.decode()
    ok = True
    if cp.returncode != 0:
        ok = False
    return ok, stdoutlines


def init_status():
    global scr
    global widths
    global cols
    global rows
    global passwords
    global mounts
    global ticklers
    global nfs_server_running

    ok, stdoutlines = run_command(['/bin/mount'])
    if not ok:
        print(f'ERROR: unable to init:\n{stdoutlines}')
        exit(1)
    mounts = {}
    patt = re.compile('^([^ ]+) on ([^ ]+) type ([^ ]+) \\((.*)\\)')
    for line in stdoutlines:
        m = patt.search(line)
        if m:
            newm = {
                'path': m.group(2),
                'source': m.group(1),
                'type': m.group(3),
                'flags': m.group(4)
            }
            mounts[m.group(2)] = newm

    luks = {}
    de: os.DirEntry
    for de in os.scandir('/dev/mapper'):
        if de.name.startswith('luks-'):
            luks[de.name] = True

    ticklers = {}
    ok, stdoutlines = run_command(['tickler', 'status'])
    if ok:
        patt = re.compile('^interval ([0-9]+) path ([^ ]+) count ([0-9]+)$')
        for line in stdoutlines:
            m = patt.search(line)
            if m:
                newtickler = {
                    'interval': m.group(1),
                    'path': m.group(2),
                    'count': m.group(3)
                }
                ticklers[m.group(2)] = newtickler
    else:
        raise PfkFsErrorStatus(f'tickler error:\n{stdoutlines}')

    for fs in pfk_fs_config.fs_list:
        if fs.passgrp:
            if not passwords.get(fs.passgrp, None):
                passwords[fs.passgrp] = ''

        # figure out if luks is open
        if fs.luks:
            if luks.get(fs.luks, None):
                fs.open = True
            else:
                fs.open = False
        else:
            # if it's not encrypted, it's not 'closed'
            fs.open = True

        # figure out if fs is mounted
        if mounts.get(fs.mntpt, None):
            fs.checked = True
            fs.mounted = True
        else:
            fs.mounted = False

    nfs_servers = 0
    for de in os.scandir('/proc'):
        try:
            pid = int(de.name)
            with open(f'/proc/{pid}/comm') as f:
                comm = f.readline()
                if comm.strip() == "nfsd":
                    nfs_servers += 1
        except ValueError:
            # skip, some things in /proc are not integers
            pass
        except FileNotFoundError:
            # skip, sometimes processes exit as we read them
            pass
    nfs_server_running = nfs_servers > 0

    # reset all online flags to False and re-walk lsblk
    for fs in pfk_fs_config.fs_list:
        fs.online = False
    cmd = ['/bin/lsblk', '-fJ']
    ok, stdoutlines = run_command(cmd, False)
    if not ok:
        print(f'lblk failed:\n{stdoutlines}')
    else:
        data = json.loads(stdoutlines)

        def do_bd(obj):
            if 'uuid' in obj:
                uuid = obj['uuid']
                if uuid:
                    for fs2 in pfk_fs_config.fs_list:
                        if fs2.UUID == uuid:
                            fs2.online = True
                            if fs2.mounted:
                                for fs3 in pfk_fs_config.fs_list:
                                    if fs3.depends == fs2.name:
                                        fs3.online = True
                            break
            if 'children' in obj:
                for child in obj['children']:
                    do_bd(child)

        if 'blockdevices' in data:
            for bd in data['blockdevices']:
                do_bd(bd)


def draw_table(selected: int):
    global scr
    global widths
    global cols
    global rows
    global passwords
    global mounts
    global ticklers
    global nfs_server_running

    scr.addstr(0, cols.num, '##', curses.A_UNDERLINE)
    scr.addstr(0, cols.name, 'name', curses.A_UNDERLINE)
    if widths.depends > 0:
        scr.addstr(0, cols.depends, 'depends', curses.A_UNDERLINE)
    if widths.luksnfs > 0:
        scr.addstr(0, cols.luksnfs, 'luks/nfs', curses.A_UNDERLINE)
    if widths.passgrp > 0:
        scr.addstr(0, cols.passgrp, 'p', curses.A_UNDERLINE)
    if widths.open > 0:
        scr.addstr(0, cols.open, 'open', curses.A_UNDERLINE)
    # ✓✔√
    if widths.checked > 0:
        scr.addstr(0, cols.checked, '✓ed', curses.A_UNDERLINE)
    scr.addstr(0, cols.mntpt, 'mnt point', curses.A_UNDERLINE)
    scr.addstr(0, cols.mounted, 'mntd', curses.A_UNDERLINE)
    scr.addstr(0, cols.tickle, 'tckl', curses.A_UNDERLINE)
    rownum = 1
    selected_fs = None
    for fs in pfk_fs_config.fs_list:
        scr.move(rownum, 0)
        scr.clrtoeol()
        if rownum == selected:
            selected_fs = fs
            scr.attron(curses.A_REVERSE)
        scr.addstr(rownum, cols.num, f'{rownum:2d}')
        scr.addstr(rownum, cols.name, f'{fs.name}')
        if fs.depends:
            scr.addstr(rownum, cols.depends, f'{fs.depends}')

        if not fs.online:
            scr.addstr(rownum, cols.luksnfs, 'OFFLINE')
        elif fs.luks:
            scr.addstr(rownum, cols.luksnfs, f'{fs.luks}')
            if fs.open:
                scr.addstr(rownum, cols.open, 'yes')
            else:
                scr.addstr(rownum, cols.open, 'no')
        elif fs.nfs:
            scr.addstr(rownum, cols.luksnfs, f'{fs.nfs}')
        if fs.passgrp:
            scr.addstr(rownum, cols.passgrp, f'{fs.passgrp}')
        scr.addstr(rownum, cols.mntpt, f'{fs.mntpt}')
        if fs.online:
            if not fs.nfs and widths.checked > 0:
                if fs.checked:
                    scr.addstr(rownum, cols.checked, 'yes')
                else:
                    scr.addstr(rownum, cols.checked, 'no')
            if fs.mounted:
                scr.addstr(rownum, cols.mounted, 'yes')
            else:
                scr.addstr(rownum, cols.mounted, 'no')
            if fs.tickle:
                t = ticklers.get(fs.mntpt, None)
                if t:
                    scr.addstr(rownum, cols.tickle, 'yes')
                else:
                    scr.addstr(rownum, cols.tickle, 'no')

        scr.attroff(curses.A_REVERSE)
        rownum += 1
    rownum += 1
    selected_passnum = None
    for p in passwords.keys():
        if selected == 100+p:
            selected_passnum = p
            scr.attron(curses.A_REVERSE)
        pw = passwords[p]
        loaded = pw if len(pw) > 0 else "no"
        scr.addstr(rownum, 0, f'password {p} loaded: {loaded}')
        scr.attroff(curses.A_REVERSE)
        scr.clrtoeol()
        rownum += 1
    rownum += 1
    rows.menuline = rownum
    scr.move(rownum, 0)
    if selected_fs:
        if not selected_fs.open:
            passnum = selected_fs.passgrp
            if passnum:
                password = passwords[passnum]
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
    if pfk_fs_config.manage_nfs_server:
        scr.move(rownum, 0)
        scr.clrtoeol()
        if nfs_server_running:
            scr.addstr(rownum, 0, "NFS server running  (<S> stop)")
        else:
            scr.addstr(rownum, 0, "NFS server NOT running (<S> start)")
        rownum += 1
    rows.cursorline = rownum
    rownum += 1
    rows.statusline = rownum
    scr.move(rows.statusline, 0)
    if selected_fs:
        if selected_fs.output:
            scr.addstr(selected_fs.output)
    scr.move(rows.cursorline, 0)


def draw_output(fs):
    global scr
    global rows
    scr.move(rows.statusline, 0)
    scr.clrtobot()
    scr.addstr(fs.output)
    scr.refresh()


def load_password(selected: int):
    global scr
    global passwords
    passnum = selected - 100
    if passnum not in passwords:
        scr.addstr(rows.cursorline, 0, f'  pass# should be one of: [')
        for p in passwords.keys():
            scr.addstr(f'{p},')
        scr.addstr(']')
    else:
        scr.addstr(rows.cursorline, 0, 'enter password: ')
        curses.echo()
        passphrase = scr.getstr()
        curses.noecho()
        passwords[passnum] = passphrase  # passphrase.decode()
        scr.move(rows.cursorline, 0)
        scr.clrtoeol()
    pass


def open_luks(selected: int):
    global scr
    entnum = selected-1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        if fs.luks:
            password = passwords[fs.passgrp]
            if len(password) == 0:
                fs.output = f'   password {fs.passgrp} is not loaded!'
            else:
                passfile = tempfile.NamedTemporaryFile(delete=False)
                passfile.write(password)
                passfile.close()
                if fs.imgfile:
                    source = fs.imgfile
                else:
                    source = f'UUID={fs.UUID}'
                cmd = ['/sbin/cryptsetup', 'open', '--type', 'luks',
                       '--key-file', passfile.name, source, fs.luks]
                fs.output = f'running command: {" ".join(cmd)}\n'
                draw_output(fs)
                ok, stdoutlines = run_command(cmd, False)
                if not ok:
                    fs.output += '  ERROR:\n'
                fs.output += stdoutlines
                os.unlink(passfile.name)
        else:
            fs.output = f'   {fs.name} is not encrypted'


def fsck(selected: int):
    global scr
    entnum = selected - 1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        if fs.luks:
            source = f'/dev/mapper/{fs.luks}'
        else:
            source = f'UUID={fs.UUID}'

        cmd = ['/sbin/fsck', '-y', source]
        fs.output = f'running command: {" ".join(cmd)}\n'
        draw_output(fs)
        ok, stdoutlines = run_command(cmd, False)
        if not ok:
            fs.output += '   ERROR:\n'
            fs.checked = False
        else:
            fs.checked = True
        fs.output += stdoutlines


def mount(selected: int):
    global scr
    global rows
    entnum = selected - 1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        if fs.luks:
            source = f'/dev/mapper/{fs.luks}'
        elif fs.UUID:
            source = f'UUID={fs.UUID}'
        else:
            source = fs.nfs

        cmd = ['/bin/mount', source, fs.mntpt]
        fs.output = f'running command: {" ".join(cmd)}\n'
        draw_output(fs)
        ok, stdoutlines = run_command(cmd, False)
        if not ok:
            fs.output += '  ERROR:\n'
        fs.output += stdoutlines

        if fs.tickle:
            cmd = ['tickler', 'add', '30 ', fs.mntpt]
            runout = f'running command: {" ".join(cmd)}\n'
            fs.output += runout
            scr.addstr(runout)
            ok, stdoutlines = run_command(cmd, False)
            if not ok:
                fs.output += '  ERROR:\n'
            fs.output += stdoutlines


def umount(selected: int):
    global scr
    entnum = selected - 1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        fs.output = ''
        if fs.tickle:
            cmd = ['tickler', 'remove', fs.mntpt]
            fs.output = f'running command: {" ".join(cmd)}\n'
            draw_output(fs)
            ok, stdoutlines = run_command(cmd, False)
            if not ok:
                fs.output += '  ERROR:\n'
            fs.output += stdoutlines

        cmd = ['/bin/umount', fs.mntpt]
        fs.output += f'running command: {" ".join(cmd)}\n'
        draw_output(fs)
        ok, stdoutlines = run_command(cmd, False)
        if not ok:
            fs.output += '  ERROR:\n'
        fs.output += stdoutlines


def close_luks(selected: int):
    global scr
    entnum = selected - 1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        if fs.luks:
            cmd = ['/sbin/cryptsetup', 'close', fs.luks]
            fs.output = f'running command: {" ".join(cmd)}\n'
            draw_output(fs)
            ok, stdoutlines = run_command(cmd, False)
            if not ok:
                scr.addstr('  ERROR:\n')
            fs.output += stdoutlines
        else:
            fs.output = f'  {fs.name} is not encrypted'


def toggle_nfs_server():
    if not pfk_fs_config.manage_nfs_server:
        return
    if nfs_server_running:
        cmd = 'stop'
    else:
        cmd = 'start'
    ok, stdoutlines = run_command(['systemctl', cmd, 'nfs-server'], False)
    if not ok:
        scr.addstr('  ERROR:\n')
    else:
        scr.addstr('  done!\n')
    scr.addstr(stdoutlines)


# def main(argv: list[str]):
def main():
    global scr
    selected = 101
    while True:
        init_status()
        draw_table(selected)
        scr.refresh()
        rd, wr, ex = select.select([0], [], [], 1)
        scr.move(rows.cursorline, 0)
        scr.clrtobot()
        if len(rd) > 0:
            ch = scr.getch()
            # scr.addstr(f'got ch {ch} ')
            # scr.refresh()
            # time.sleep(1)
            if ch == ord('q'):
                break
            elif ch == 10:
                if selected > 100:
                    load_password(selected)
            elif ch == ord('o'):
                if selected < 100:
                    open_luks(selected)
            elif ch == ord('f'):
                if selected < 100:
                    fsck(selected)
            elif ch == ord('m'):
                if selected < 100:
                    mount(selected)
            elif ch == ord('u'):
                if selected < 100:
                    umount(selected)
            elif ch == ord('c'):
                if selected < 100:
                    close_luks(selected)
            elif ch == ord('S'):
                toggle_nfs_server()
            elif ch == curses.KEY_UP:
                if selected == 101:
                    selected = len(pfk_fs_config.fs_list)
                elif selected > 1:
                    selected -= 1
            elif ch == curses.KEY_DOWN:
                if selected < len(pfk_fs_config.fs_list):
                    selected += 1
                elif selected == len(pfk_fs_config.fs_list):
                    selected = 101
                elif selected < (100+len(passwords)):
                    selected += 1
            elif ch == 9:  # tab
                if selected > 100:
                    selected = 1
                else:
                    selected = 101

    return 0


if __name__ == '__main__':
    # resize window to 80x40, move to home, clear to end
    tc = pfkterm.TermControl(use_curses=True, rows=40, cols=80)
    measure_widths()
    scr = tc.scr
    r = 1
    err = None
    try:
        # r = main(sys.argv)
        r = main()
    except KeyboardInterrupt:
        err = Exception('interrupted by keyboard')
    except curses.error as e:
        err = e
    except Exception as e:
        err = e
    del tc
    if err:
        raise err
    exit(r)
