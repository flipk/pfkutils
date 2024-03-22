#!/usr/bin/env python3

# TODO if exe doesn't exist, it crashes instead of printing graceful error.
# TODO if there's a problem with mounting or unmounting, tickler status
#      is inconsistent.

import curses
import select
import subprocess
import re
import tempfile
from _pfk_fs_screen import Screen
import pfk_fs_config
import os
import time

rows = {}
passwords = {}
mounts = {}
luks = {}


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
    fs_list = pfk_fs_config.fs_list
    scr = Screen(fs_list)
    r = 1
    # noinspection PyBroadException
    try:
        # r = main(sys.argv)
        r = main()
    except:
        # clean up curses before raising the exception
        # so the exception can print out cleanly.
        del scr
        raise
    del scr
    exit(r)
