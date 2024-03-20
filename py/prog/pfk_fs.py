#!/usr/bin/env python3

# TODO if exe doesn't exist, it crashes instead of printing graceful error.
# TODO if there's a problem with mounting or unmounting, tickler status
#      is inconsistent.

import curses
import select
import subprocess
import re
import tempfile
import pfk_fs_config
import os
import time

widths = {}
cols = {}
rows = {}
passwords = {}
mounts = {}
luks = {}


def measure_widths():
    # measure the ones that are in fs
    for f in ['name', 'UUID', 'imgfile', 'depends', 'luks', 'mntpt', 'nfs']:
        width = 0
        for fs in pfk_fs_config.fs_list:
            if f not in fs:
                print(f'ERROR: field {f} is missing from {fs["name"]}')
                exit(1)
            if fs[f]:
                if width < len(fs[f]):
                    width = len(fs[f])
        widths[f] = width

    has_checked = False
    for fs in pfk_fs_config.fs_list:
        if fs['UUID'] or fs['imgfile']:
            has_checked = True

    widths['num'] = 2  # not in fs
    widths['pass'] = 1 if widths['luks'] > 0 else 0  # not a string
    widths['open'] = 4 if widths['luks'] > 0 else 0  # not in fs
    widths['checked'] = 3 if has_checked else 0  # not in fs
    widths['mounted'] = 7  # not in fs

    pos = 0
    for f in ['num', 'name', 'depends', 'luks',
              'pass', 'open', 'checked', 'nfs', 'mntpt', 'mounted']:
        cols[f] = pos
        if widths[f] > 0:
            pos += widths[f] + 1


def run_command(cmd: list[str]):
    try:
        cp = subprocess.run(cmd,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=30)
    except subprocess.TimeoutExpired:
        print(f'ERROR: subprocess {cmd} timeout')
        time.sleep(5)
        exit(1)
    stdoutlines = cp.stdout.decode().split('\n')
    ok = True
    if cp.returncode != 0:
        ok = False
    return ok, stdoutlines


def init_status():
    global mounts
    global luks
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

    for fs in pfk_fs_config.fs_list:
        if fs['pass']:
            if isinstance(fs['pass'], int):
                if not passwords.get(fs['pass'], None):
                    passwords[fs['pass']] = ''
            else:
                print(f'ERROR: field "pass" of {fs["name"]} is not an integer')
                exit(1)

        # figure out if luks is open
        if fs['luks']:
            if luks.get(fs['luks'], None):
                fs['open'] = True
            else:
                fs['open'] = False
        else:
            # if it's not encrypted, it's not 'closed'
            fs['open'] = True

        # figure out if fs is mounted
        if mounts.get(fs['mntpt'], None):
            fs['checked'] = True
            fs['mounted'] = True
        else:
            fs['mounted'] = False
            if not fs.get('checked', None):
                fs['checked'] = False


def draw_table(selected: int):
    global scr
    global widths
    scr.addstr(0, cols['num'], '##', curses.A_UNDERLINE)
    scr.addstr(0, cols['name'], 'name', curses.A_UNDERLINE)
    if widths['depends'] > 0:
        scr.addstr(0, cols['depends'], 'depends', curses.A_UNDERLINE)
    if widths['luks'] > 0:
        scr.addstr(0, cols['luks'], 'luks', curses.A_UNDERLINE)
    if widths['pass'] > 0:
        scr.addstr(0, cols['pass'], 'p', curses.A_UNDERLINE)
    if widths['open'] > 0:
        scr.addstr(0, cols['open'], 'open', curses.A_UNDERLINE)
    # ✓✔√
    if widths['checked'] > 0:
        scr.addstr(0, cols['checked'], '✓ed', curses.A_UNDERLINE)
    if widths['nfs'] > 0:
        scr.addstr(0, cols['nfs'], 'nfs', curses.A_UNDERLINE)
    scr.addstr(0, cols['mntpt'], 'mnt point', curses.A_UNDERLINE)
    scr.addstr(0, cols['mounted'], 'mounted', curses.A_UNDERLINE)
    rownum = 1
    selected_fs = None
    for fs in pfk_fs_config.fs_list:
        if rownum == selected:
            selected_fs = fs
            scr.attron(curses.A_REVERSE)
        scr.addstr(rownum, cols['num'], f'{rownum:2d}')
        scr.addstr(rownum, cols['name'], f'{fs["name"]}')
        if fs['depends']:
            scr.addstr(rownum, cols['depends'], f'{fs["depends"]}')
        if fs['luks']:
            scr.addstr(rownum, cols['luks'], f'{fs["luks"]}')
        else:
            scr.addstr(rownum, cols['luks'], 'n/a')
        if fs['pass']:
            scr.addstr(rownum, cols['pass'], f'{fs["pass"]}')
        if fs['open']:
            scr.addstr(rownum, cols['open'], 'yes')
        else:
            scr.addstr(rownum, cols['open'], 'no ')
        if widths['checked'] > 0:
            if fs['checked']:
                scr.addstr(rownum, cols['checked'], 'yes')
            else:
                scr.addstr(rownum, cols['checked'], 'no ')
        if widths['nfs'] > 0:
            scr.addstr(rownum, cols['nfs'], f'{fs["nfs"]}')
        scr.addstr(rownum, cols['mntpt'], f'{fs["mntpt"]}')
        if fs['mounted']:
            scr.addstr(rownum, cols['mounted'], 'yes')
        else:
            scr.addstr(rownum, cols['mounted'], 'no ')
        scr.attroff(curses.A_REVERSE)
        rownum += 1
    rownum += 1
    selected_passnum = None
    for p in passwords.keys():
        if selected == 100+p:
            selected_passnum = p
            scr.attron(curses.A_REVERSE)
        pw = passwords[p]
        loaded = pw if len(pw) > 0 else "no "
        scr.addstr(rownum, 0, f'password {p} loaded: {loaded}')
        scr.attroff(curses.A_REVERSE)
        rownum += 1
    rownum += 1
    rows['menuline'] = rownum
    scr.move(rownum, 0)
    if selected_fs:
        if not selected_fs['open']:
            passnum = selected_fs['pass']
            if passnum:
                password = passwords[passnum]
                if len(password) == 0:
                    scr.addstr(f'(pass {passnum} is not loaded) ')
                else:
                    scr.addstr('<o>pen-luks ')
        else:
            if widths['checked'] > 0:
                if not selected_fs['checked']:
                    scr.addstr('<f>sck ')
            if not selected_fs['mounted']:
                if selected_fs['luks']:
                    scr.addstr('<c>lose-luks ')
                scr.addstr('<m>ount ')
            else:
                scr.addstr('<u>mount ')
    elif selected_passnum:
        scr.addstr('<enter> load password ')
    scr.addstr(' <q>uit')
    scr.clrtoeol()
    rownum += 1
    rows['cursorline'] = rownum
    scr.move(rownum, 0)


def load_password(selected: int):
    global scr
    global passwords
    passnum = selected - 100
    if passnum not in passwords:
        scr.addstr(rows['cursorline'], 0, f'  pass# should be one of: [')
        for p in passwords.keys():
            scr.addstr(f'{p},')
        scr.addstr(']')
    else:
        scr.addstr(rows['cursorline'], 0, 'enter password: ')
        curses.echo()
        passphrase = scr.getstr()
        curses.noecho()
        passwords[passnum] = passphrase  # passphrase.decode()
        scr.move(rows['cursorline'], 0)
        scr.clrtoeol()
    pass


def open_luks(selected: int):
    global scr
    entnum = selected-1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        if fs['luks']:
            source = f'/dev/mapper/{fs["luks"]}'
        else:
            source = f'UUID={fs["UUID"]}'
        scr.addstr(f'    {fs["mntpt"]}: {source}\n')
        if fs['luks']:
            password = passwords[fs['pass']]
            if len(password) == 0:
                scr.addstr(f'ERROR: password {fs["pass"]} is not loaded!')
            else:
                passfile = tempfile.NamedTemporaryFile(delete=False)
                passfile.write(password)
                passfile.close()
                if fs['imgfile']:
                    source = fs["imgfile"]
                else:
                    source = f'UUID={fs["UUID"]}'
                cmd = ['/sbin/cryptsetup', 'open', '--type', 'luks',
                       '--key-file', passfile.name, source, fs['luks']]
                scr.addstr(f'running command: ')
                for c in cmd:
                    scr.addstr(f'{c} ')
                scr.addstr('\n')
                scr.refresh()
                ok, stdoutlines = run_command(cmd)
                if ok:
                    scr.addstr('  opened!')
                    init_status()
                else:
                    scr.addstr('  ERROR:\n')
                    for line in stdoutlines:
                        scr.addstr(f'{line}\n')
                os.unlink(passfile.name)
        else:
            scr.addstr(f'  ERROR: {fs["name"]} is not encrypted')
    else:
        scr.addstr(f'  ERROR: did not find entry # {entnum}')


def fsck(selected: int):
    global scr
    entnum = selected - 1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        if fs['luks']:
            source = f'/dev/mapper/{fs["luks"]}'
        else:
            source = f'UUID={fs["UUID"]}'
        scr.addstr(f'    {fs["mntpt"]}: {source}\n')
        cmd = ['/sbin/fsck', '-y', source]
        scr.addstr(f'running command: ')
        for c in cmd:
            scr.addstr(f'{c} ')
        scr.addstr('\n')
        scr.refresh()
        ok, stdoutlines = run_command(cmd)
        for line in stdoutlines:
            scr.addstr(f'{line}\n')
        if ok:
            scr.addstr('  done!')
            fs['checked'] = True
            init_status()
        else:
            fs['checked'] = False
            scr.addstr('  FAILED\n')
    else:
        scr.addstr(f'  ERROR: did not find entry # {entnum}')


def mount(selected: int):
    global scr
    entnum = selected - 1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        if fs['luks']:
            source = f'/dev/mapper/{fs["luks"]}'
        elif fs['UUID']:
            source = f'UUID={fs["UUID"]}'
        else:
            source = fs['nfs']
        scr.addstr(f'    {fs["mntpt"]}: {source}\n')

        cmd = ['/bin/mount', source, fs['mntpt']]
        scr.addstr(f'running command: ')
        for c in cmd:
            scr.addstr(f'{c} ')
        scr.addstr('\n')
        scr.refresh()
        ok, stdoutlines = run_command(cmd)
        if ok:
            scr.addstr('  mounted!')
            init_status()
        else:
            scr.addstr('  ERROR:\n')
            for line in stdoutlines:
                scr.addstr(f'{line}\n')

        if fs['tickle']:
            cmd = ['tickler', 'add', '30 ', fs['mntpt']]
            scr.addstr(f'running command: ')
            for c in cmd:
                scr.addstr(f'{c} ')
            scr.addstr('\n')
            ok, stdoutlines = run_command(cmd)
            if not ok:
                scr.addstr('  ERROR:\n')
            for line in stdoutlines:
                scr.addstr(f'{line}\n')

    else:
        scr.addstr(f'  ERROR: did not find entry # {entnum}')


def umount(selected: int):
    global scr
    entnum = selected - 1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        if fs['luks']:
            source = f'/dev/mapper/{fs["luks"]}'
        else:
            source = f'UUID={fs["UUID"]}'
        scr.addstr(f'    {fs["mntpt"]}: {source}\n')

        if fs['tickle']:
            cmd = ['tickler', 'remove', fs['mntpt']]
            scr.addstr(f'running command: ')
            for c in cmd:
                scr.addstr(f'{c} ')
            scr.addstr('\n')
            ok, stdoutlines = run_command(cmd)
            if not ok:
                scr.addstr('  ERROR:\n')
            for line in stdoutlines:
                scr.addstr(f'{line}\n')

        cmd = ['/bin/umount', fs['mntpt']]
        scr.addstr(f'running command: ')
        for c in cmd:
            scr.addstr(f'{c} ')
        scr.addstr('\n')
        scr.refresh()
        ok, stdoutlines = run_command(cmd)
        if ok:
            scr.addstr('  unmounted!')
        else:
            scr.addstr('  ERROR:\n')
            for line in stdoutlines:
                scr.addstr(f'{line}\n')
    else:
        scr.addstr(f'  ERROR: did not find entry # {entnum}')


def close_luks(selected: int):
    global scr
    entnum = selected - 1
    fs = None
    if 0 <= entnum < len(pfk_fs_config.fs_list):
        fs = pfk_fs_config.fs_list[entnum]
    if fs:
        if fs['luks']:
            source = f'/dev/mapper/{fs["luks"]}'
        else:
            source = f'UUID={fs["UUID"]}'
        scr.addstr(f'    {fs["mntpt"]}: {source}\n')
        # cryptsetup close $volname
        if fs['luks']:
            cmd = ['/sbin/cryptsetup', 'close', fs['luks']]
            scr.addstr(f'running command: ')
            for c in cmd:
                scr.addstr(f'{c} ')
            scr.addstr('\n')
            scr.refresh()
            ok, stdoutlines = run_command(cmd)
            if ok:
                scr.addstr('  closed!')
                init_status()
            else:
                scr.addstr('  ERROR:\n')
                for line in stdoutlines:
                    scr.addstr(f'{line}\n')
        else:
            scr.addstr(f'  ERROR: {fs["name"]} is not encrypted')
    else:
        scr.addstr(f'  ERROR: did not find entry # {entnum}')


# def main(argv: list[str]):
def main():
    global scr
    selected = 101
    while True:
        init_status()
        draw_table(selected)
        scr.refresh()
        rd, wr, ex = select.select([0], [], [], 60)
        scr.move(rows['cursorline'], 0)
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
    measure_widths()
    scr = curses.initscr()
    curses.noecho()
    curses.cbreak()
    scr.keypad(True)
    r = 1
    # noinspection PyBroadException
    try:
        # r = main(sys.argv)
        r = main()
    except:
        # clean up curses before raising the exception
        # so the exception can print out cleanly.
        scr.keypad(False)
        del scr
        curses.nocbreak()
        curses.echo()
        curses.endwin()
        raise
    scr.keypad(False)
    del scr
    curses.nocbreak()
    curses.echo()
    curses.endwin()
    exit(r)
