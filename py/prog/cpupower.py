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

import curses
import sys
import select


def main(argv: list[str]):

    # figure out how many CPUs there are.
    with open('/sys/devices/system/cpu/online', mode='r') as f:
        presents = f.readline()
        presents = presents.split('-')
        num_cpus = int(presents[1])+1
        col_mod = int(num_cpus / 2)

    # now we know the size of the cpu table, figure out where various
    # lines are going to go.
    statusline = ((num_cpus-1) % col_mod) + 2
    govline = statusline + 1
    menuline = govline + 2

    # grab a list of the available cpu governors
    with open('/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors',
              mode='r') as f:
        governors = f.readline()
        governors = governors.strip().split(' ')

    # the hard limits on cpu frequency don't change, so go grab them now.
    cpu_limits = []
    for n in range(0, num_cpus):
        with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/cpuinfo_min_freq',
                  mode="r") as f:
            min_freq = f.readline()
            min_freq = int(min_freq.strip())
        with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/cpuinfo_max_freq',
                  mode="r") as f:
            max_freq = f.readline()
            max_freq = int(max_freq.strip())
        cpu_limits.append({'min': min_freq, 'max': max_freq})

    # sample cpu0 to see what the 'current' governor is.
    # assumption: all the cpus are set the same.
    with open('/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor',
              mode='r') as f:
        current_gov = f.readline()

    # print a few things
    stdscr.addstr(govline, 0, 'available governors:\n ')
    for i in range(0, len(governors)):
        stdscr.addstr(f'{governors[i]} ')
    stdscr.addstr(statusline, 0, f'current governor:  {current_gov}')
    stdscr.addstr(menuline, 0, '(p) powersave   (P) Performance   (q) quit')

    while True:

        # print the cpu table.
        for n in range(0, num_cpus):
            with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/scaling_cur_freq',
                      mode="r") as f:
                freq = f.readline()
                freq = int(freq.strip()) / 1000
            with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/scaling_min_freq',
                      mode="r") as f:
                scale_min = f.readline()
                scale_min = int(scale_min.strip())
            with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/scaling_max_freq',
                      mode="r") as f:
                scale_max = f.readline()
                scale_max = int(scale_max.strip())
            stdscr.addstr(n % col_mod, int(n/col_mod) * 40,
                          f'{n:2d}: '
                          f'{cpu_limits[n]["min"]/1000}-{cpu_limits[n]["max"]/1000} '
                          f'{scale_min/1000}-{scale_max/1000} '
                          f'{freq:6.1f}')
            stdscr.clrtoeol()

        # actually output to the screen.
        stdscr.refresh()

        # wait for a keypress or a polling interval.
        rd, wr, ex = select.select([0], [], [], 0.2)
        if len(rd) > 0:
            # there was a keypress, do something.
            c = stdscr.getch()
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
                    stdscr.addstr(statusline, 0, f'current mode:  {current_gov}')
                    stdscr.clrtoeol()
            elif c == ord('P'):
                for n in range(0, num_cpus):
                    with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/scaling_max_freq',
                              mode='w') as f:
                        f.write(f'{cpu_limits[n]["max"]}')
                    current_gov = 'performance'
                    with open(f'/sys/devices/system/cpu/cpu{n}/cpufreq/scaling_governor',
                              mode='w') as f:
                        f.write(current_gov)
                    stdscr.addstr(statusline, 0, f'current mode:  {current_gov}')
                    stdscr.clrtoeol()

    # stdscr.addstr(20, 0, f'{curses.COLS} x {curses.LINES} got ch {c}', curses.A_NORMAL)
    return 0


if __name__ == '__main__':
    stdscr = curses.initscr()
    curses.noecho()
    curses.cbreak()
    stdscr.keypad(True)
    r = 1
    try:
        r = main(sys.argv)
    except KeyboardInterrupt:
        pass
    stdscr.keypad(False)
    del stdscr
    curses.nocbreak()
    curses.echo()
    curses.endwin()
    exit(r)
