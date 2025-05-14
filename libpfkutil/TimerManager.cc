
#include "TimerManager.h"

#include <iostream>
#include <vector>

#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// Forward declaration for logging prefix
namespace TimerManagerDetail {
    const char* LOG_PREFIX = "[TimerManager] ";
}

#ifdef __TIMER_MANAGER_TEST_MAIN__
#define VERBOSE 1
#else
#define VERBOSE 0
#endif

TimerManager :: TimerManager(long tick_interval_ms)
    : exit_flag_(false),
      tick_interval_us_(tick_interval_ms * 1000),
      current_tick_(0), next_timer_id_(0)
{
    if (tick_interval_ms <= 0)
        throw std::invalid_argument("Tick interval must be positive.");

    if (pipe(pipe_fd_) == -1)
        throw std::runtime_error(
            std::string(TimerManagerDetail::LOG_PREFIX)
            + "Failed to create pipe: " + strerror(errno));

    try
    {
        helper_thread_ = std::thread(&TimerManager::helper_thread_function,
                                     this);
    }
    catch (const std::system_error& e)
    {
        close(pipe_fd_[0]);
        close(pipe_fd_[1]);
        throw std::runtime_error(
            std::string(TimerManagerDetail::LOG_PREFIX)
            + "Failed to spawn helper thread: " + e.what());
    }
    if (VERBOSE)
        std::cout << TimerManagerDetail::LOG_PREFIX
                  << "Initialized with tick interval: " << tick_interval_ms
                  << " ms." << std::endl;
}

TimerManager :: ~TimerManager()
{
    // std::cout << TimerManagerDetail::LOG_PREFIX
    //   << "Destructor called." << std::endl;
    exit_flag_.store(true, std::memory_order_release);

    char buf = 'x'; // Dummy byte to wake up select
    ssize_t written = write(pipe_fd_[1], &buf, 1);
    if (written == -1)
        // Non-critical error for shutdown, but log it.
        std::cerr << TimerManagerDetail::LOG_PREFIX
                  << "Warning: Failed to write to pipe in destructor: "
                  << strerror(errno) << std::endl;

    if (helper_thread_.joinable())
        helper_thread_.join();

    if (pipe_fd_[0] != -1)
        close(pipe_fd_[0]);
    if (pipe_fd_[1] != -1)
        close(pipe_fd_[1]);
    pipe_fd_[0] = pipe_fd_[1] = -1;

    // cancel all outstanding timers.
    {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        for (auto &t : active_timers_)
            t.second.callback(t.second.id, /*cancel*/ true);
        if (VERBOSE)
            std::cout << TimerManagerDetail::LOG_PREFIX
                      << "Cleared remaining active timers." << std::endl;
    }
    if (VERBOSE)
        std::cout << TimerManagerDetail::LOG_PREFIX
                  << "Destruction complete." << std::endl;
}

TimerManager :: TimerId TimerManager :: set_timer(TimerCallback callback,
                                                  uint32_t ticks_in_future)
{
    if (!callback)
        throw std::invalid_argument("Timer callback cannot be null.");

    TimerId new_id = next_timer_id_.fetch_add(1, std::memory_order_relaxed);
    uint64_t current_tick = current_tick_.load(std::memory_order_acquire);
    uint64_t expiration_absolute_tick = current_tick + ticks_in_future;

    TimerObject timer_obj;
    timer_obj.id = new_id;
    timer_obj.callback = std::move(callback);
    timer_obj.expiration_tick = expiration_absolute_tick;

    {
        std::lock_guard<std::mutex> lock(timers_mutex_);
        active_timers_.emplace(new_id, std::move(timer_obj));
    }
    if (VERBOSE)
        std::cout << TimerManagerDetail::LOG_PREFIX
                  << "Set timer ID " << new_id
                  << " to expire at tick " << expiration_absolute_tick
                  << " (in " << ticks_in_future << " ticks from current "
                  << current_tick << ")" << std::endl;
    return new_id;
}

bool TimerManager :: cancel_timer(TimerManager::TimerId id)
{
    std::lock_guard<std::mutex> lock(timers_mutex_);
    auto it = active_timers_.find(id);
    if (it != active_timers_.end())
    {
        it->second.callback(it->second.id, /*cancel*/true);
        active_timers_.erase(it);
        return true;
    }
    std::cout << TimerManagerDetail::LOG_PREFIX
              << "Timer ID " << id
              << " not found for cancellation." << std::endl;
    return false;
}


void TimerManager :: helper_thread_function()
{
    if (VERBOSE)
        std::cout << TimerManagerDetail::LOG_PREFIX
                  << "Helper thread started." << std::endl;
    fd_set read_fds;
    const int read_pipe_fd = pipe_fd_[0];
    int nfds = read_pipe_fd + 1;

    while (!exit_flag_.load(std::memory_order_acquire))
    {
        FD_ZERO(&read_fds);
        FD_SET(read_pipe_fd, &read_fds);

        struct timeval tv;
        tv.tv_sec = tick_interval_us_ / 1000000;
        tv.tv_usec = tick_interval_us_ % 1000000;

        int retval = select(nfds, &read_fds, nullptr, nullptr, &tv);

        if (exit_flag_.load(std::memory_order_acquire))
            // time to exit!
            break;

        if (retval < 0)
        {
            if (errno == EINTR)
            {
                if (VERBOSE)
                    std::cout << TimerManagerDetail::LOG_PREFIX
                              << "select() interrupted by signal, retrying."
                              << std::endl;
                continue;
            }
            std::cerr << TimerManagerDetail::LOG_PREFIX
                      << "select() error in helper thread: "
                      << strerror(errno) << ". Exiting thread." << std::endl;
            break;
        }
            
        uint64_t effective_current_tick = current_tick_.load(
            std::memory_order_relaxed);

        struct TBT {
            TBT(TimerId id, TimerCallback cb) : id_(id), cb_(cb) { }
            TimerId id_;
            TimerCallback cb_;
            void call(void) const { cb_(id_, /*cancel*/false); }
        };
        std::vector<TBT> callbacks_to_run;
        {
            std::lock_guard<std::mutex> lock(timers_mutex_);
            for (auto it = active_timers_.begin();
                 it != active_timers_.end();
                 /* manual increment */)
            {
                if (it->second.expiration_tick <= effective_current_tick)
                {
                    callbacks_to_run.emplace_back(it->second.id,
                                                  it->second.callback);
                    it = active_timers_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        for (const auto& cb : callbacks_to_run)
        {
            try
            {
                cb.call();
            }
            catch (const std::exception& e)
            {
                std::cerr << TimerManagerDetail::LOG_PREFIX
                          << "Timer callback threw std::exception: "
                          << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << TimerManagerDetail::LOG_PREFIX
                          << "Timer callback threw unknown exception."
                          << std::endl;
            }
        }
            
        // Increment the master tick count for the next interval
        // if we are not exiting
        if (!exit_flag_.load(std::memory_order_acquire))
            current_tick_.fetch_add(1, std::memory_order_relaxed);
    }
    if (VERBOSE)
        std::cout << TimerManagerDetail::LOG_PREFIX
                  << "Helper thread exiting." << std::endl;
}


#ifdef __TIMER_MANAGER_TEST_MAIN__

// --- Example Usage ---
void my_timer_callback_1(TimerManager::TimerId id, bool cancel)
{
    if (cancel)
        printf("ERROR: callback 1 (id %u) canceled\n", id);
    else
        printf("OK: callback 1 (id %u) expired!\n", id);
}

void my_timer_callback_2(TimerManager::TimerId id, bool cancel)
{
    if (cancel)
        printf("ERROR: callback 2 (id %u) canceled\n", id);
    else
        printf("OK: callback 2 (id %u) expired!\n", id);

}

void my_timer_callback_3_cancel_test(TimerManager::TimerId id, bool cancel)
{
    if (cancel)
        printf("OK: callback 3 (id %u) canceled\n", id);
    else
        printf("ERROR: callback 3 (id %u) expired!\n", id);
}



int main()
{
    std::cout << "Main: Program started." << std::endl;
    try
    {
        TimerManager timer_manager(100); // 100ms tick interval

        std::cout << "Main: Setting timers." << std::endl;
        TimerManager::TimerId id1 = timer_manager.set_timer(
            my_timer_callback_1, 5);  // Fires after 5 * 100ms = 500ms
        TimerManager::TimerId id2 = timer_manager.set_timer(
            my_timer_callback_2, 12); // Fires after 12 * 100ms = 1200ms
        TimerManager::TimerId id3_to_cancel = timer_manager.set_timer(
            my_timer_callback_3_cancel_test, 8); // Should be 800ms

        std::cout << "Main: Sleeping for 200ms..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        std::cout << "Main: Canceling timer ID " << id3_to_cancel << std::endl;
        bool canceled = timer_manager.cancel_timer(id3_to_cancel);
        if (canceled)
            std::cout << "Main: Successfully canceled timer ID "
                      << id3_to_cancel << std::endl;
        else
            std::cout << "Main: Failed to cancel timer ID "
                      << id3_to_cancel << std::endl;

        // Attempt to cancel a non-existent timer
        TimerManager::TimerId non_existent_id = 9999;
        std::cout << "Main: Attempting to cancel non-existent timer ID "
                  << non_existent_id << std::endl;
        canceled = timer_manager.cancel_timer(non_existent_id);
        if (!canceled)
            std::cout << "Main: Correctly reported timer ID "
                      << non_existent_id
                      << " not found for cancellation." << std::endl;

        std::cout << "Main: Sleeping for 2000ms to let other timers fire..."
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        std::cout << "Main: Timer tests complete. "
                  << "TimerManager will now be destroyed." << std::endl;

    }
    catch (const std::exception& e)
    {
        std::cerr << "Main: Exception caught: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Main: Program finished successfully." << std::endl;
    return 0;
}

#endif // __TIMER_MANAGER_TEST_MAIN__
