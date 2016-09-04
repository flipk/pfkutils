#include "th.h"
#include "simnet.h"

/*
 * the clock task runs in a loop incrementing the time.
 * it is the lowest prio task so it only runs when all 
 * tasks for the current tick have completed.
 * the tick isr is the highest priority task, so when the
 * low-prio tick task wakes it up, nothing else runs
 * until the tick isr has completed its work.
 *
 * so this is what happens: all nodes have gone into 
 * net_receive, and blocked. the tick isr task is blocked.
 *
 * the tick thread gets to run.  it increments the clock,
 * and resumes the tick isr. the tick isr, being high priority,
 * is caused to run immediately.  it calls the net isr, which
 * processes all packets which are in delay and need to go
 * to a node.  the net isr then resumes every node which has
 * data to receive, but because they are lower prio than the
 * clock isr, they don't run until the net isr completes all
 * of this work.  when the clock isr finally suspends itself,
 * each of the nodes wakes up in turn and processes its packets.
 * 
 * when all nodes have finally processed their packets and
 * have again blocked in net_receive, the low-prio clock task
 * finally gets to run again, and the cycle repeats.
 */

static int time_now;
static int clock_isr_tid;
static int clock_done;

static void clock_tick    (void *dummy);
static void clock_tick_isr(void *dummy);

void
clock_init()
{
	clock_done = time_now = 0;
	/* lowest priority task */
	th_create(clock_tick,     0,  1, "clock task");
	/* highest priority task */
	th_create(clock_tick_isr, 0, 31, "clock/net isr");
}

static void
clock_tick_isr(void *dummy)
{
	clock_isr_tid = th_tid();
	while (!clock_done)
	{
		th_suspend(0);
		clock_done = net_clock_isr();
	}
}

static void
clock_tick(void *dummy)
{
	while (!clock_done)
	{
		time_now ++;
		th_resume(clock_isr_tid);
	}
}

int
clock_time_now(void)
{
	return time_now;
}
