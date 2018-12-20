
#include <string.h>
#include <stdio.h>

#include "timers.h"

#define BUMP(x) statistics.x++

pfkTimers :: pfkTimers(void)
{
   uint16_t i;
   timer_entry * te;

   current_tick = 0;
   memset(timers, 0, sizeof(timers));
   for (i=0; i < MAX_NUMBER_TIMERS; i++)
   {
      te = &timers[i];
      te->list_next = i+1;
      te->list_prev = INVALID_INDEX;
      te->my_index = i;
   }
   timers[MAX_NUMBER_TIMERS-1].list_next = INVALID_INDEX;
   timer_freestk_top = 0;
   for (i=0; i < TIMER_WHEEL_SIZE; i++)
      timer_wheel[i] = INVALID_INDEX;

   LOCK_INIT();
}

pfkTimers :: ~pfkTimers(void)
{
    LOCK_DESTROY();
}

pfkTimers :: TIMER_ID
pfkTimers :: set( TIMER_HANDLER_FUNC func,
                  void * arg, uint32_t ticks )
{
   uint16_t timer_index;
   uint16_t * timer_ptr;
   uint32_t wheel_index;
   timer_entry * te, * nte;

   BUMP(timer_set);

   if (func == NULL)
   {
      BUMP(timer_null_funcptr);
      fprintf(stderr, "pfkTimers :: set : function pointer is null");
      return INVALID_TIMERID;
   }

   if (ticks == 0)
   {
      /*
       * the function needs to be called now.
       * we're not going to consume a timer slot for it.
       * problem is, we can't return INVALID_TIMERID, because
       * that means the wrong thing to our caller: that the
       * timer will never be invoked due to out of timers.
       * so, return a timer id that is valid but already expired,
       * such as the previous sequence number on timer slot 0.
       */

      BUMP(timer_expire_0);

      func(arg);
      return (uint32_t)timers[0].sequence - 1;
   }

   LOCK();
   timer_index = timer_freestk_top;
   if (timer_index >= MAX_NUMBER_TIMERS)
   {
      UNLOCK();
      BUMP(timer_out_of_timers);
      return INVALID_TIMERID;
   }
   te = &timers[timer_index];
   timer_freestk_top = te->list_next;

   te->func = func;
   te->arg = arg;
   te->my_index = timer_index;
   te->list_prev = INVALID_INDEX;
   te->expire_tick = current_tick + ticks;
   wheel_index = te->expire_tick % TIMER_WHEEL_SIZE;

   timer_ptr = &timer_wheel[wheel_index];

   if (*timer_ptr >= MAX_NUMBER_TIMERS)
   {
      *timer_ptr = timer_index;
      te->list_next = INVALID_INDEX;
   }
   else
   {
      nte = &timers[*timer_ptr];
      te->list_next = nte->my_index;
      nte->list_prev = timer_index;
      *timer_ptr = timer_index;
   }

   UNLOCK();

   return ((uint32_t)timer_index << 16) + (uint32_t)te->sequence;
}

pfkTimers :: CANCEL_RET
pfkTimers :: cancel(TIMER_ID timer_id)
{
   timer_entry * te;
   uint16_t timer_index = (uint16_t)(timer_id >> 16);
   uint16_t sequence    = (uint16_t)(timer_id & 0xffff);

   BUMP(timer_cancel);

   if (timer_index >= MAX_NUMBER_TIMERS)
      return TIMER_INVALID_ID;

   te = &timers[timer_index];

   LOCK();
   if (te->sequence != sequence)
   {
      UNLOCK();
      BUMP(timer_cancel_not_set);
      return TIMER_NOT_SET;
   }

   if (te->list_next < MAX_NUMBER_TIMERS)
   {
      timer_entry * nte = &timers[te->list_next];
      nte->list_prev = te->list_prev;
   }

   if (te->list_prev < MAX_NUMBER_TIMERS)
   {
      timer_entry * pte = &timers[te->list_prev];
      pte->list_next = te->list_next;
   }
   else
   {
      int wheel_index = te->expire_tick % TIMER_WHEEL_SIZE;
      timer_wheel[wheel_index] = te->list_next;
   }

   te->sequence++;
   te->func = NULL;
   te->list_prev = INVALID_INDEX;
   te->list_next = timer_freestk_top;
   timer_freestk_top = timer_index;
   UNLOCK();

   return TIMER_CANCELLED;
}

void
pfkTimers :: tick_announce(void)
{
   int wheel_index;
   uint16_t * timer_ptr;
   uint16_t timer_index;
   timer_entry * te;
   uint16_t execute_list = INVALID_INDEX;
   uint16_t * execute_next = &execute_list;

   LOCK();

   current_tick++;

   wheel_index = current_tick % TIMER_WHEEL_SIZE;

   timer_ptr = &timer_wheel[wheel_index];
   timer_index = *timer_ptr;

   while (timer_index < MAX_NUMBER_TIMERS)
   {
      te = &timers[timer_index];
      if (te->expire_tick == current_tick)
      {
         te->sequence++;

         timer_index = te->list_next;

         if (te->list_next < MAX_NUMBER_TIMERS)
         {
            timer_entry * nte = &timers[te->list_next];
            nte->list_prev = te->list_prev;
         }

         *timer_ptr = timer_index;

         te->list_next = *execute_next;
         *execute_next = te->my_index;
         execute_next = &te->list_next;
      }
      else
      {
         timer_ptr = &te->list_next;
         timer_index = *timer_ptr;
      }
   }

   UNLOCK();

   te = NULL;
   timer_index = execute_list;
   while (timer_index < MAX_NUMBER_TIMERS)
   {
      te = &timers[timer_index];
      timer_index = te->list_next;
      BUMP(timer_expire);
      te->func( te->arg );
      te->func = NULL;
      te->list_prev = INVALID_INDEX;
   }

   LOCK();

   if (te)
   {
      /*
       * te was left pointing at the last item of the
       * execute list. so to free all the items on the
       * execute list, lets just spice it into the head
       * of the free list.
       */

      te->list_next = timer_freestk_top;
      timer_freestk_top = execute_list;
   }

   UNLOCK();
}

#ifdef __INCLUDE_TIMER_TEST__

void func1(void *arg)
{
    printf("func1 called\n");
}

int
main()
{
    pfkTimers  timers;

    pfkTimers::TIMER_ID  id1;
    pfkTimers::TIMER_ID  id2;

    id1 = timers.set( &func1, NULL, 3 );
    id2 = timers.set( &func1, NULL, 5 );

    timers.tick_announce();
    timers.tick_announce();
    timers.tick_announce();

    timers.cancel(id1);
    timers.cancel(id2);

    timers.tick_announce();
    timers.tick_announce();
    timers.tick_announce();

    return 0;
}

#endif // __INCLUDE_TIMER_TEST__
