/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <signal.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <ucontext.h>

struct SignalBacktraceInfo
{
#ifdef __cplusplus
public:
    SignalBacktraceInfo(void);
    ~SignalBacktraceInfo(void);
    void clear(void);
    void desc_print(const char *format...);
#endif

    int fatal; // actually "bool"

#define SignalBacktrace_MAX_DESCRIPTION 8192
    char description[SignalBacktrace_MAX_DESCRIPTION];
    int desc_len;

    int sig; // 0 means no sig (backtrace_now was called)
    siginfo_t siginfo_data;
    ucontext_t *uc;

#define SignalBacktrace_MAX_TRACE_ENTRIES 40
    int trace_size;
    void *trace[SignalBacktrace_MAX_TRACE_ENTRIES];
    char **messages;

#ifdef __arm__
    int trace_size2;
    void *trace2[SignalBacktrace_MAX_TRACE_ENTRIES];
    char **messages2;
#endif
};

// recommend checking info->sig for
// SIGTERM, SIGHUP, SIGINT, or SIGQUIT (info->fatal == false)
// and handling them specially, for instance SIGTERM doesn't
// have to exit immediately, it could trigger a safe shutdown sequence
// in the application.
typedef void (*SignalBacktraceHandler)(
    const struct SignalBacktraceInfo *info);

#ifdef __cplusplus

class SignalBacktrace
{
    pid_t pid;
    char process_name[64];
    SignalBacktraceHandler handler;
    SignalBacktraceInfo  info;
    void do_backtrace(void);
    static void default_user_handler(const struct SignalBacktraceInfo *info);
    static void signal_handler(int sig, siginfo_t *info, void *uc);
    static SignalBacktrace * instance;
    SignalBacktrace(void);
    ~SignalBacktrace(void);
public:
    static SignalBacktrace * get_instance(void);
    static void cleanup(void);
    void register_handler(const char *process_name,
                          SignalBacktraceHandler new_handler);
    void backtrace_now(const char *reason);
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

void signal_backtrace_init(const char *process_name,
                           SignalBacktraceHandler new_handler);
void signal_backtrace_cleanup(void);
void signal_backtrace_now(const char *reason);


#ifdef __cplusplus
}
#endif
