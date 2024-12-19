
#define THREADSATES_FILE "/tmp/thread_states.bin"

#define THREAD_STATES_LIST \
    THREAD_STATES_ITEM(thread1_iterations)  \
    THREAD_STATES_ITEM(thread1_situation1)  \
    THREAD_STATES_ITEM(thread1_situation2)  \
    THREAD_STATES_ITEM(thread1_lineno)

struct thread_states {
#define THREAD_STATES_ITEM(item)  int item ;
    THREAD_STATES_LIST ;
#undef  THREAD_STATES_ITEM
};

extern thread_states * thstates;
void thread_states_sync(void);
void thread_states_init(bool init_contents=true);

#define THSTATE_BUMP(field)                     \
    if (thstates != NULL)                       \
    {                                           \
        thstates->field++;                      \
        thread_states_sync();                   \
    }

#define THSTATE_LINE(field)                     \
    if (thstates != NULL)                       \
    {                                           \
        thstates->field = __LINE__;             \
        thread_states_sync();                   \
    }

#define THSTATE_SET(field, value)               \
    if (thstates != NULL)                       \
    {                                           \
        thstates->field = value;                \
        thread_states_sync();                   \
    }
