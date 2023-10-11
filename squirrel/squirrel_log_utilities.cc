
#include "squirrel_log.h"
#include <sys/syscall.h>
#include <execinfo.h>
#include <math.h>
#include <iomanip>

std::string SquirrelLog :: hexdump_buffer(const uint8_t *buf, int len)
{
    std::ostringstream  ss;
    ss << "len " << len << " ";
    for (int pos = 0; pos < len; pos++)
        ss << std::hex << std::setfill('0') << std::setw(2)
           << (uint32_t) buf[pos];
    return ss.str();
}

//static
void SquirrelLog :: set_thread_name(const char *name)
{
    pthread_setname_np(pthread_self(), name);
    LOG_F("SQUIRREL", INFO, "thread %s started", name);
}

double SquirrelLog :: timeval_to_double(const struct timeval &tv)
{
    return (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0);
}

void SquirrelLog :: double_time_to_timeval(struct timeval &tv, double t)
{
    double s = floor(t);
    double us = (t - s) * 1000000.0;
    tv.tv_sec  = (typeof(tv.tv_sec )) s;
    tv.tv_usec = (typeof(tv.tv_usec)) round(us);
}

void SquirrelLog :: timeval_to_timestr(char time_text[timestr_len],
                               const struct timeval &tv)
{
    time_t sec = (time_t) tv.tv_sec;
    struct tm tm;
    gmtime_r(&sec, &tm);
    static_assert(strlen("2023/01/01 00:00:00.000000") < timestr_len);
    strftime(time_text, timestr_len, "%Y/%m/%d %H:%M:%S", &tm);
    sprintf(time_text + strlen(time_text), ".%06d", (int) tv.tv_usec);
}

// static
int SquirrelLog :: sql_busy(void *arg, int invocations)
{
    if (0) // diagnostic only! enable this when you want to know
           // how often this actually happens.
        fprintf(stderr, "SQL BUSY pid %d tid %d count %d\n",
                (int) getpid(), gettid(),
                invocations);
    usleep(1);
    return 1; // try again.
}

//static
void SquirrelLog :: init_sql_tables(sqlite3 *pdb,
                                    const std::string &table_name,
                                    int version_in_file,
                                    int version_in_code)
{
    // nothing for now
}

// static
void SquirrelLog :: log_sql_upd(void *arg, sqlite3_stmt *stmt)
{
//    SquirrelLog * stf = (SquirrelLog*) arg;
    if (verbose_sql)
    {
        char * sql = sqlite3_expanded_sql(stmt);
        printf("SQL: %s\n", sql);
        sqlite3_free(sql);
    }
}

// static
void SquirrelLog :: log_sql_get(void *arg, sqlite3_stmt *stmt)
{
//    SquirrelLog * stf = (SquirrelLog*) arg;
    if (verbose_sql)
    {
        char * sql = sqlite3_expanded_sql(stmt);
        printf("SQL: %s\n", sql);
        sqlite3_free(sql);
    }
}

// static
void SquirrelLog :: log_sql_row(void *arg, const std::string &msg)
{
//    SquirrelLog * stf = (SquirrelLog*) arg;
//    fprintf(stderr, "** SQL ROW: %s\n", msg.c_str());
}

// static
void SquirrelLog :: log_sql_err(void *arg, const std::string &msg)
{
//    SquirrelLog * stf = (SquirrelLog*) arg;
    fprintf(stderr, "** SQL ERROR: %s\n", msg.c_str());
}

// static
int SquirrelLog :: gettid(void)
{
    return syscall(SYS_gettid);
}

//static
size_t SquirrelLog :: format_siginfo (char *dest, size_t sz,
                                      const siginfo_t *info)
{
    FILE * f = fmemopen(dest, sz, "w");

    fprintf(f, "----------------SIGINFO----------------\n");
    fprintf(f, "signo: %d   errno: %d    code: 0x%x    ",
            info->si_signo, info->si_errno, info->si_code);
    fprintf(f, "fault address: %p\n", info->si_addr);
    fprintf(f, "(see asm-generic/siginfo.h si_codes)\n");
    fprintf(f, "---------------------------------------\n");

    size_t ret = ftell(f);
    fclose(f);
    return ret;
}

//static
size_t SquirrelLog :: format_ucontext(char *dest, size_t sz,
                                      const void *_uc)
{
    FILE * f = fmemopen(dest, sz, "w");
    const uint8_t * uc = (const uint8_t *) _uc;
//    const ucontext_t *uc = (const ucontext_t *) _uc;

    fprintf(f, "--------------------------------UCONTEXT"
            "--------------------------------\n");

    for (int i = 0; i < sizeof(ucontext_t); i++)
    {
        if (i % 24 == 0)
            fprintf(f, "%03x: ", i);
        fprintf(f, "%02x ", uc[i]);
        if (i % 24 == 23)
            fprintf(f, "\n");
    }
    fprintf(f, "\n-------------------------------"
            "-----------------------------------------\n");

    int ret = ftell(f);
    fclose(f);
    return ret;
}

//static
void SquirrelLog :: signal_handler_trace(int sig, siginfo_t *info, void *uc)
{
    bool          raw_bt = false;
    const char *  hdr = "CRASH";
    char          siginfo[16384];
    char        * si = siginfo;
    size_t        siremain = sizeof(siginfo) - 1;

    // in case there is no info nor uc, put a nul.
    si[0] = 0;

    if (info)
    {
        size_t sigot = format_siginfo(si, siremain, info);
        // in case there is no uc, put a nul.
        si[sigot] = 0;
        si += sigot;
        siremain -= sigot;
    }
    if (uc)
    {
        size_t sigot = format_ucontext(si, siremain, uc);
        // make sure it's nul terminated.
        si[sigot] = 0;
    }

    if (in_signal_handler)
    {
        // fault during a fault! handle cleanly.
        raw_bt = true;
        hdr = "CRASH DURING A CRASH";
    }
    else
    {
        in_signal_handler = true;
        if (instance)
        {
            LOG_FBT("SQUIRREL", CRIT, "CRASH: signal %d\n%s", sig, siginfo);
        }
        else
        {
            raw_bt = true;
            // no  instance? fault outside of regular code!
            // handle cleanly.
        }
    }

    if (raw_bt)
    {
        static const int bt_buffer_size = 96;
        void *bt_buffer[bt_buffer_size];
        int bt_size = backtrace(bt_buffer, bt_buffer_size);

        // hopefully someone is logging the stdout/stderr.
        fprintf(stderr, "\n\n\n----------%s-------------\n", hdr);
        fprintf(stderr, "%s\n", siginfo);
        backtrace_symbols_fd(bt_buffer, bt_size, /*stderr*/2);
        fprintf(stderr, "\n----------%s-------------\n\n\n", hdr);
    }
    else
    {
        // if for some reason we recursed on the signal handler,
        // or SquirrelLog isn't set up yet, don't call the user's
        // callback function cuz we need to die quickly.
        if (exit_callback_function)
            exit_callback_function(sig, /*fatal*/true);
    }

    // if no handler, or handler returns, this is not a clean exit.
    signal(SIGABRT, SIG_DFL);
    abort();
}

//static
void SquirrelLog :: signal_handler_exit(int sig)
{
    LOG_F("SQUIRREL", INFO, "got signal %d, exiting", sig);

    if (exit_callback_function)
        exit_callback_function(sig, /*fatal*/false);
    else
    {
        // if no handler, this is not a clean exit.
        exit(1);
    }
}
