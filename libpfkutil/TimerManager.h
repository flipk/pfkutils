
/*

prompt to Google Gemini 2.5 Pro:

write a c++ class for managing timers. this will be in use on linux.

on construction, the class should spawn a helper thread and the
destructor should cause the thread to exit (waiting for the thread to
be collected before returning).

the helper thread spins in a while loop blocking on a select call with
a timeval populated to e.g. 100 milliseconds (call this a "tick"; the
interval of the tick should be specified as an argument to the
constructor). a common method used elsewhere in the code where this
will be used for controlling exit of a thread is for the constructor
to construct a unix "pipe" and add the read-end of that pipe to the
fd_set of select for-read. when the destructor wants to cause exit of
the thread, it sets a boolean flag in the object and then writes 1
byte to the write-end of the pipe to trigger 'select' to wake up
immediately, detect the flag, and then return on its own. the
destructor can then block while joining the thread. if there are any
remaining un-expired timers, delete them in the destructor.

the class should have some kind of data structure (a list or map or
whatever is appropriate) containing a set of pending timer
objects. each timer object should have a closure callback, a unique
identifier of some kind, and whatever else is required by the thread
to manage countdown of the timer.

each time the select completes, the thread should efficiently examine
the set of timer objects determining which timers are due to expire in
this tick. if a timer is due, the closure in it should be invoked and
the timer object deleted.

the unique identifier should be done in such a way that a given value
will not be reused for a long time in the future, perhaps with a
32-bit counter that increments for every new timer.

there should be a method in the class called "set_timer" which is used
to set a new timer which takes a closure and a number of ticks in the
future as arguments and returns a unique identifier for the timer
object. this method should add the newly created timer object to the
set of timer objects, and it should do it in a thread-safe manner.

another method called "cancel_timer" should take the unique identifier
returned by set_timer, search for the timer object identified by that
identifier, and delete it without invoking the closure. it should do
this in a thread-safe manner. it should return a bool true if the
timer id was found and false if it was not.

modifications by PFK:

- callbacks take a timerid and a 'cancel'
- callbacks are invoked when canceled but with cancel=true
- callbacks are invoked during destructor with cancel=true
- H and CC files separated.
- much reformating for readability!
- test main func is optional by ifdef

*/

#ifndef __TIMER_MANAGER_H__
#define __TIMER_MANAGER_H__

#include <inttypes.h>
#include <mutex>
#include <map>
#include <functional>
#include <thread>
#include <atomic>


class TimerManager {
public:
    using TimerId = uint32_t;
    using TimerCallback = std::function<void(TimerId,bool cancel)>;

    // tick_interval_ms: The duration of one "tick" in milliseconds.
    TimerManager(long tick_interval_ms);

    ~TimerManager();

    // Deleted copy constructor and assignment operator
    TimerManager(const TimerManager&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;

    // Sets a new timer.
    // callback: The function to call when the timer expires.
    // ticks_in_future: The number of ticks from now when the timer
    //                  should expire.  A value of 0 means it will be
    //                  considered for expiration on the very next
    //                  processing cycle of the helper thread.
    // Returns: A unique identifier for the timer.
    TimerId set_timer(TimerCallback callback, uint32_t ticks_in_future);

    // Cancels an existing timer.
    // id: The unique identifier of the timer to cancel.
    // Returns: true if the timer was found and canceled, false otherwise.
    bool cancel_timer(TimerId id);

private:
    struct TimerObject {
        TimerId id;
        TimerCallback callback;
        uint64_t expiration_tick;
    };

    std::thread helper_thread_;
    std::atomic<bool> exit_flag_;
    int pipe_fd_[2]{-1, -1}; // pipe_fd_[0] is read, pipe_fd_[1] is write

    std::mutex timers_mutex_; // Protects active_timers_
    std::map<TimerId, TimerObject> active_timers_;

    std::atomic<uint64_t> current_tick_;
    std::atomic<TimerId> next_timer_id_;

    long tick_interval_us_; // Tick interval in microseconds for select()

    void helper_thread_function();
};

#endif // __TIMER_MANAGER_H__
