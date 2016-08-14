
#include "tiddb.h"
#include "fileparser.h"
#include <stdlib.h>

using namespace pfktop;
using namespace std;

tidEntry :: tidEntry(pid_t _tid, pid_t _pid,
                     const std::string &_path,
                     tidEntry * _parent /*= NULL*/)
    : tid(_tid), pid(_pid), pathToDir(_path), parent(_parent),
      stamp(false), first_update(true), db(false)
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

    nf = fp.parse(pathToDir + "/stat");
    // if there was any error in parsing this file, the number
    // of fields will not be 52.  since we're not error checking
    // for failure to open file, this covers that case too.
    if (nf == 52)
    {
        stat_line = fp.get_line();
        cmd = fp[1];
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
        }
        utime_prev = utime;
        stime_prev = stime;
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
