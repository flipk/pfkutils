#!/usr/bin/env python3

# https://docs.python.org/3/library/curses.html
# watch -n 10 upower -i /org/freedesktop/UPower/devices/battery_BAT0
# sudo cpupower frequency-info
# sudo cpupower frequency-set -g performance
# sudo cpupower frequency-set -g powersave
# sudo cpupower frequency-set --max 800MHz
# sudo cpupower frequency-set --max 3600MHz
# sudo powertop
# on fedora, 'cpupower' is in kernel-tools, and powertop is in 'powertop'

import os
import sys

sys.path.append(f'{os.environ["HOME"]}/proj/pfkutils/py/lib')

import time
import select
import curses
import pfkterm
# import sys
# import stat


# def main(argv: list[str]):
def main():

    for cpudir in ['/sys/devices/system/cpu']:
        if os.path.exists(cpudir):
            break
        cpudir = None

    if not cpudir:
        scr.addstr(0, 0, 'ERROR: cannot find /sys cpu dir')
        scr.refresh()
        time.sleep(2)
        return 1

    # figure out how many CPUs there are.
    with open(f'{cpudir}/online', mode='r') as f:
        presents = f.readline()
        presents = presents.split('-')
        num_cpus = int(presents[1])+1
        col_mod = int(num_cpus / 2)

    # alternative:
    # (try/except FileNotFoundError) cpudirstat = os.stat(cpudir)
    # if stat.S_ISDIR(cpudirstat.st_mode):
    for hwmondir in ['/sys/devices/platform/coretemp.0/hwmon/hwmon',
                     '/sys/devices/platform/coretemp.0/hwmon/hwmon0',
                     '/sys/devices/platform/coretemp.0/hwmon/hwmon1',
                     '/sys/devices/platform/coretemp.0/hwmon/hwmon2',
                     '/sys/devices/platform/coretemp.0/hwmon/hwmon3',
                     '/sys/devices/platform/coretemp.0/hwmon/hwmon4',
                     '/sys/devices/platform/coretemp.0/hwmon/hwmon5',
                     '/sys/devices/platform/coretemp.0/hwmon/hwmon6']:
        if os.path.exists(f'{hwmondir}/temp2_label'):
            break
        hwmondir = None

    if not hwmondir:
        scr.addstr(0, 0, 'ERROR: cannot find /sys hwmon dir')
        scr.refresh()
        time.sleep(2)
        return 1

    # find all the temperature files.
    tempfiles = []
    for de in os.scandir(hwmondir):
        if de.name.endswith('_input'):
            tempfiles.append(de.path)

    temp_lines = int(len(tempfiles) / 8)
    temp_start_line = int(((num_cpus-1) % col_mod) + 2)

    # now we know the size of the cpu table, figure out where various
    # lines are going to go.
    statusline = temp_start_line + temp_lines + 2
    govline = statusline + 1
    menuline = govline + 2

    # grab a list of the available cpu governors
    with open(f'{cpudir}/cpu0/cpufreq/scaling_available_governors',
              mode='r') as f:
        governors = f.readline()
        governors = governors.strip().split(' ')

    # the hard limits on cpu frequency don't change, so go grab them now.
    cpu_limits = []
    for n in range(0, num_cpus):
        with open(f'{cpudir}/cpu{n}/cpufreq/cpuinfo_min_freq',
                  mode="r") as f:
            min_freq = f.readline()
            min_freq = int(min_freq.strip())
        with open(f'{cpudir}/cpu{n}/cpufreq/cpuinfo_max_freq',
                  mode="r") as f:
            max_freq = f.readline()
            max_freq = int(max_freq.strip())
        cpu_limits.append({'min': min_freq, 'max': max_freq})

    # sample cpu0 to see what the 'current' governor is.
    # assumption: all the cpus are set the same.
    with open(f'{cpudir}/cpu0/cpufreq/scaling_governor',
              mode='r') as f:
        current_gov = f.readline()

    # print a few things
    scr.addstr(govline, 0, 'available governors:\n ')
    for i in range(0, len(governors)):
        scr.addstr(f'{governors[i]} ')
    scr.addstr(statusline, 0, f'current governor:  {current_gov}')
    scr.addstr(menuline, 2, '(p) powersave   (P) Performance   (q) quit')

    while True:
        with open(f'{cpudir}/cpu0/cpufreq/base_frequency',
                  mode="r") as f:
            basefreq = f.readline()
            basefreq = basefreq.strip()

        # print the cpu table.
        for n in range(0, num_cpus):
            with open(f'{cpudir}/cpu{n}/cpufreq/scaling_cur_freq',
                      mode="r") as f:
                freq = f.readline()
                freq = freq.strip()
                if freq == basefreq:
                    # gross heuristic: if it reads as identical
                    # to base_freq, it's probably idle. we can't
                    # tell for sure because i can't find a file
                    # in /sys that actually tells you. i just found
                    # the if-statement in the kernel driver that falls
                    # back to base_freq if the cpu is idle, so....
                    freqstr = '     -'
                else:
                    freq = int(freq) / 1000
                    freqstr = f'{freq:6.1f}'
            with open(f'{cpudir}/cpu{n}/cpufreq/scaling_max_freq',
                      mode="r") as f:
                scale_max = f.readline()
                scale_max = int(scale_max.strip())
            scr.addstr(n % col_mod, int(n / col_mod) * 40,
                       f'{n:2d}: '
                       f'({cpu_limits[n]["min"]/1000}-{cpu_limits[n]["max"]/1000})'
                       f':{scale_max/1000} '
                       f'{freqstr}')
            scr.clrtoeol()

        # print temperatures
        temp_index = 0
        for fn in tempfiles:
            with open(fn) as f:
                temp_line = f.readline()
                temp = int(temp_line.strip()) / 1000
                scr.addstr(temp_start_line + int(temp_index / 8),
                           10 * (temp_index % 8),
                           f'{temp} ')
                temp_index += 1

        # output to the screen. leave the cursor in a nice place.
        # stdscr.move(curses.LINES-1, 0)
        scr.move(menuline, 0)
        scr.refresh()

        # wait for a keypress or a polling interval.
        rd, wr, ex = select.select([0], [], [], 0.2)
        if len(rd) > 0:
            # there was a keypress, do something.
            c = scr.getch()
            if c == ord('q'):
                break
            elif c == ord('p'):
                for n in range(0, num_cpus):
                    with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/scaling_max_freq',
                              mode='w') as f:
                        f.write(f'{cpu_limits[n]["min"]}')
                    current_gov = 'powersave'
                    with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/scaling_governor',
                              mode='w') as f:
                        f.write(current_gov)
                    scr.addstr(statusline, 0, f'current mode:  {current_gov}')
                    scr.clrtoeol()
            elif c == ord('P'):
                for n in range(0, num_cpus):
                    with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/scaling_max_freq',
                              mode='w') as f:
                        f.write(f'{cpu_limits[n]["max"]}')
                    current_gov = 'performance'
                    with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/scaling_governor',
                              mode='w') as f:
                        f.write(current_gov)
                    scr.addstr(statusline, 0, f'current mode:  {current_gov}')
                    scr.clrtoeol()

    # stdscr.addstr(20, 0, f'{curses.COLS} x {curses.LINES} got ch {c}', curses.A_NORMAL)
    return 0


if __name__ == '__main__':
    # resize window to 80x30, move to home, clear to end
    tc = pfkterm.TermControl(use_curses=True, rows=30, cols=80)
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
