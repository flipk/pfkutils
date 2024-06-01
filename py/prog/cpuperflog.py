#!/usr/bin/env python3

import sys
import os
import select
import time

# stress-ng --matrix 32 --matrix-size 64 --maximize -t 10s

hwmondir = '/sys/devices/platform/coretemp.0/hwmon/hwmon3'
# e.g. : /sys/devices/system/cpu/cpufreq/policy5/scaling_cur_freq
freqdir = '/sys/devices/system/cpu/cpufreq/policy'
scalefile = 'scaling_cur_freq'
basefreqfile = '/sys/devices/system/cpu/cpu0/cpufreq/base_frequency'
interval = 0.2

# find all the temperature files.
templabels = []
tempfiles = {}
for de in os.scandir(hwmondir):
    if de.name.endswith('_input'):
        temppath = de.path
        labelpath = temppath.replace('_input', '_label')
        with open(labelpath, mode='r') as fd:
            label = fd.readline()
            label = label.strip()
            templabels.append(label)
        tempfiles[label] = temppath
templabels.sort()

# find all the freq files
freqfiles = []
for col in range(0, 128):
    freqfile = f'{freqdir}{col}/{scalefile}'
    if os.access(freqfile, os.R_OK):
        freqfiles.append(freqfile)
with open(basefreqfile, mode='r') as fd:
    basefreq = fd.readline()
    basefreq = basefreq.strip()

print('"time","timestr","mintemp","maxtemp",',end='')
for lab in templabels:
    print(f'"{lab}",', end='')
for ff in range(0, len(freqfiles)):
    print(f'"freq {ff}",', end='')
print('')
while True:
    temps = ''
    mintemp = 200.0
    maxtemp = 0.0
    for lab in templabels:
        f = tempfiles[lab]
        with open(f, mode='r') as fd:
            temp_line = fd.readline()
            temp = int(temp_line.strip()) / 1000
            temps += f'{temp},'
            if maxtemp < temp:
                maxtemp = temp
            if mintemp > temp:
                mintemp = temp
    freqs = ''
    for ff in freqfiles:
        with open(ff, mode='r') as fd:
            freq = fd.readline()
            freq = freq.strip()
            if freq == basefreq:
                freq = 0
            freq = float(freq) / 1000
            freqs += f'{freq:.1f},'
    now = time.clock_gettime(time.CLOCK_REALTIME)
    nowstr = time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(now))
    print(f'{now:06f},{nowstr},{mintemp},{maxtemp},{temps}{freqs}')
    sys.stdout.flush()
    rd, wr, ex = select.select([0], [], [], interval)
    if len(rd) > 0:
        break

print(' --- gnuplot commands:\n'
      'set datafile separator ","\n'
      'set xdata time\n'
      'set format x "%H:%M:%S" time\n'
      'set timefmt "%Y/%m/%d %H:%M:%S"')

print(f"plot "
      f"'temps.csv' using 2:3 with lines title 'min temp',"
      f" 'temps.csv' using 2:4 with lines title 'max temp' ")

print('plot ', end='')
col = 5
first = True
for lab in templabels:
    if not first:
        print(',', end='')
    else:
        first = False
    print(f"'temps.csv' using 2:{col} with lines title '{lab}'", end='')
    col += 1
print('')

print('plot ', end='')
first = True
for n in range(0, len(freqfiles)):
    if not first:
        print(',', end='')
    else:
        first = False
    print(f"'temps.csv' using 2:{col} with lines title 'core {n}'", end='')
    col += 1
print('')
