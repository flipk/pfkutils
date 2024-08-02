/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "tiddb.h"
#include "fileparser.h"
#include <stdlib.h>

using namespace pfktop;
using namespace std;

tidEntry :: tidEntry(pid_t _tid, pid_t _pid,
                     const std::string &_path,
                     tidEntry * _parent /*= NULL*/)
    : tid(_tid), pid(_pid), pathToDir(_path), parent(_parent),
      stamp(false), first_update(true), db(false), avg10s(0)
{
//    cout << "new pid " << pid << " tid " << tid << "\r\n";
}

tidEntry :: ~tidEntry(void)
{
//    cout << "deleting pid " << pid << " tid " << tid << "\r\n";
}

void
tidEntry :: update(void)
{
    if (stamp)
        // already updated, dont do it twice.
        return;

    stamp = true;
    fileParser fp;
    int nf;

// see https://www.kernel.org/doc/Documentation/filesystems/proc.txt
// format of the "stat" file in /proc.
// Field          Content
//  pid           process id
//  tcomm         filename of the executable
//  state    2    state (R is running, S is sleeping, D is sleeping in an
//                 uninterruptible wait, Z is zombie, T is traced or stopped)
//  ppid          process id of the parent process
//  pgrp          pgrp of the process
//  sid           session id
//  tty_nr        tty the process uses
//  tty_pgrp      pgrp of the tty
//  flags         task flags
//  min_flt       number of minor faults
//  cmin_flt      number of minor faults with child's
//  maj_flt       number of major faults
//  cmaj_flt      number of major faults with child's
//  utime    13    user mode jiffies
//  stime    14    kernel mode jiffies
//  cutime        user mode jiffies with child's
//  cstime        kernel mode jiffies with child's
//  priority 17   priority level
//  nice     18   nice level
//  num_threads   number of threads
//  it_real_value (obsolete, always 0)
//  start_time    time the process started after system boot
//  vsize         virtual memory size
//  rss      23   resident set memory size
//  rsslim        current limit in bytes on the rss
//  start_code    address above which program text can run
//  end_code      address below which program text can run
//  start_stack   address of the start of the main process stack
//  esp           current value of ESP
//  eip           current value of EIP
//  pending       bitmap of pending signals
//  blocked       bitmap of blocked signals
//  sigign        bitmap of ignored signals
//  sigcatch      bitmap of caught signals
//  0             (place holder, used to be the wchan address,
//                 use /proc/PID/wchan instead)
//  0             (place holder)
//  0             (place holder)
//  exit_signal   signal to send to parent thread on exit
//  task_cpu      which CPU the task is scheduled on
//  rt_priority   realtime priority
//  policy        scheduling policy (man sched_setscheduler)
//  blkio_ticks   time spent waiting for block IO
//  gtime         guest time of the task in jiffies
//  cgtime        guest time of the task children in jiffies
//  start_data    address above which program data+bss is placed
//  end_data      address below which program data+bss is placed
//  start_brk     address above which program heap can be expanded with brk()
//  arg_start     address above which program command line is placed
//  arg_end       address below which program command line is placed
//  env_start     address above which program environment is placed
//  env_end       address below which program environment is placed
//  exit_code     the thread's exit_code in the form reported
//                 by the waitpid system call

// linux 2.6.18 format is not documented in Documentation/
//  so here is the source that makes the output:
//        res = sprintf(buffer,"%d (%s) %c %d %d %d %d %d %lu %lu
//%lu %lu %lu %lu %lu %ld %ld %ld %ld %d 0 %llu %lu %ld %lu %lu %lu %lu %lu
//%lu %lu %lu %lu %lu %lu %lu %lu %d %d %lu %lu %llu\n",
//   0            task->pid,
//   1            tcomm,
//   2            state,
//   3            ppid,
//   4            pgid,
//   5            sid,
//   6            tty_nr,
//   7            tty_pgrp,
//   8            task->flags,
//   9            min_flt,
//   10           cmin_flt,
//   11           maj_flt,
//   12           cmaj_flt,
//   13           cputime_to_clock_t(utime),
//   14           cputime_to_clock_t(stime),
//   15           cputime_to_clock_t(cutime),
//   16           cputime_to_clock_t(cstime),
//   17           priority,
//   18           nice,
//   19           num_threads,
//   20           /* 0 in format string */
//   21           start_time,
//   22           vsize,
//   23           mm ? get_mm_rss(mm) : 0,
//   24           rsslim,
//   25           mm ? mm->start_code : 0,
//   26           mm ? mm->end_code : 0,
//   27           mm ? mm->start_stack : 0,
//   28           esp,
//   29           eip,
//   30           task->pending.signal.sig[0] & 0x7fffffffUL,
//   31           task->blocked.sig[0] & 0x7fffffffUL,
//   32           sigign      .sig[0] & 0x7fffffffUL,
//   33           sigcatch    .sig[0] & 0x7fffffffUL,
//   34           wchan,
//   35           0UL,
//   36           0UL,
//   37           task->exit_signal,
//   38           task_cpu(task),
//   39           task->rt_priority,
//   40           task->policy,
//   41           (unsigned long long)delayacct_blkio_ticks(task));

    nf = fp.parse(pathToDir + "/stat");
    stat_line = fp.get_line();
    // if there was any error in parsing this file, the number
    // of fields will not be 52.  since we're not error checking
    // for failure to open file, this covers that case too.
    // recent linux has 52 entries, 2.6.18 has 42.  the ones
    // we are interested have the same index in both formats.
    // 2.6.32 has 44 entries.
    if (nf == 52 || nf == 42 || nf == 44)
    {
        cmd = fp[1];
        if (cmd.size() > 16)
            cmd.resize(16);
        utime = strtoull(fp[13].c_str(),NULL,10);
        stime = strtoull(fp[14].c_str(),NULL,10);
        if (first_update)
        {
            first_update = false;
            utime_diff = stime_diff = 0;
            diffsum = 0;
        }
        else
        {
            utime_diff = utime - utime_prev;
            stime_diff = stime - stime_prev;
            diffsum = utime_diff + stime_diff;
            // the cpu time can exceed 100 because this process
            // may be low priority and get delayed, and the process
            // we're measuring accumulated more than 1 second's worth
            // of ticks.  cap to 99 for nice display (2 columns).
            if (diffsum > 99)
                diffsum = 99;
            history.insert(history.begin(), diffsum);
            if (history.size() > 10)
                history.resize(10);

            // sum and count the CPU history, for the 'average' column.
            int s = 0;
            int c = 0;

            for (int ind = 0; ind < history.size(); ind++)
            {
                int v = history[ind];
                if (v >= 0)
                {
                    s += (v * 100);
                    c ++;
                }
            }
            if (c > 0)
                s /= c;
            avg10s = s;
        }
        utime_prev = utime;
        stime_prev = stime;
        vsz = strtoul(fp[22].c_str(),NULL,10);
        vsz /= 1024;
        rss = strtoul(fp[23].c_str(),NULL,10);
        prio = strtol(fp[17].c_str(),NULL,10); // signed.
        state = fp[2][0];
    }
    else
    {
        // this is not an error, this happens quite naturally.
        // there's a race between reading /proc and then trying
        // to descend into the /proc/pid or /proc/pid/task dirs.
        // if the proc or thread has exited, we might hit a window
        // of time where we thought it existed but it really doesn't.
        history.insert(history.begin(), -1);
        if (history.size() > 10)
            history.resize(10);
    }
}
