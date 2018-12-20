
#include <inttypes.h>
#include <pthread.h>

class pfkTimers {

/*
 *  The top 16 bits of a timer id are an index into the timers array.
 *  The bottom 16 bits of the timer id are the sequence number.
 *  Everytime a timer entry is freed, the sequencen number is incremented.
 */

public:
    typedef void (*TIMER_HANDLER_FUNC)(void*);
    typedef uint32_t TIMER_ID;
    static const uint32_t INVALID_TIMERID = 0xFFFFFFFFUL;

    typedef enum {
        TIMER_INVALID_ID,
        TIMER_NOT_SET,
        TIMER_CANCELLED
    } CANCEL_RET;

    struct stats {
        int timer_set;
        int timer_null_funcptr;
        int timer_expire_0;
        int timer_out_of_timers;
        int timer_cancel;
        int timer_cancel_not_set;
        int timer_expire;
        stats(void) { memset((void*)this, 0, sizeof(struct stats)); }
    };
    stats statistics;

private:
    struct timer_entry {
        uint16_t  list_next;
        uint16_t  list_prev;
        uint16_t  sequence;
        uint16_t  my_index;
        uint32_t  expire_tick;
        TIMER_HANDLER_FUNC func;
        void * arg;
    };

    static const int MAX_NUMBER_TIMERS = 8000;
    static const int TIMER_WHEEL_SIZE = 4096; /* must be power of 2 */
    static const uint16_t INVALID_INDEX = 0xFFFF;

    timer_entry timers [MAX_NUMBER_TIMERS];
    uint16_t  timer_wheel [TIMER_WHEEL_SIZE];
    uint32_t  current_tick;
    uint16_t  timer_freestk_top;

    pthread_mutex_t  mutex;

    void LOCK_INIT(void)
    {
        pthread_mutex_init(&mutex, NULL);
    }
    void LOCK_DESTROY(void)
    {
        pthread_mutex_destroy(&mutex);
    }        
    void LOCK(void)
    {
        pthread_mutex_lock(&mutex);
    }
    void UNLOCK(void)
    {
        pthread_mutex_unlock(&mutex);
    }

public:
    pfkTimers(void);
    ~pfkTimers(void);
    uint32_t tick_get(void) const { return current_tick; }
    TIMER_ID set( TIMER_HANDLER_FUNC func, void * arg, uint32_t ticks );
    CANCEL_RET cancel(TIMER_ID timer_id);
    void tick_announce(void);
};
