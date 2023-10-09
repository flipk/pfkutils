#ifndef __SQUIRREL_LOG_H__
#define __SQUIRREL_LOG_H__ 1

#include "sqlite3.h"
#include SQUIRREL_DB_PROTO_HDR
#include "squirrel_db.h"
#include "thread_slinger.h"
#include "posix_fe.h"
#include <signal.h>
#include <string>
#include <sstream>
#include <vector>

class SquirrelLog
{
public:
    // this class must be initialized before it can do anything.
    // it must open the SQL database and init the tables in it.
    // if this returns false, something is wrong with the database file.
    static bool init(const std::string &dbpath, bool allow_core = false);
    // the last thing you do before you exit() is cleanup.
    static void cleanup(void);

    // set this if you want to see SQL statements printed on stderr.
    // this can be changed at any time and takes effect immediately.
    static bool verbose_sql;

    // the "level" argument to LOG* macros is this enum.
    enum Level : int
    {
        CRIT       = 0,
        ERR        = 1,
        WARN       = 2,
        INFO       = 3,
        DBG        = 4,
        TRACE      = 5,
        _USERLEVEL = 6  // user can extend this starting here.
    };

    // each unique thread should call this, so that LOG* events show
    // an accurate thread name.
    static void set_thread_name(const char *name);

    // ------------------------ FORMATTED LOGGING ------------------------
    // used by LOG_F macros.

    static void log(const char* file, int line, bool do_backtrace,
                    const char* area, int level,
                    const char *format, ...)
        __attribute__((__format__ (__printf__, 6, 7)));

    // ------------------------ STREAM LOGGING ------------------------
    // used by LOG_S macro.

    class StreamLogger
    {
    public:
        StreamLogger(const char* file, int line, bool do_backtrace,
                     const char* area, int level)
            : _file(file), _line(line), _do_backtrace(do_backtrace),
              _area(area), _level(level) { }
        ~StreamLogger(void) {
            instance->log( _file, _line, _do_backtrace,
                           _area, _level, "%s", _ss.str().c_str());
        }

        template<typename T>
        StreamLogger& operator<<(const T& t)
        { _ss << t; return *this; }

        // std::endl and iomanip
        StreamLogger& operator<<(std::ostream&(*f)(std::ostream&))
        { f(_ss); return *this; }
    private:
        const char* _file;
        int _line;
        bool _do_backtrace;
        const char* _area;
        int _level;
        std::ostringstream _ss;
    };

    // ------------------------ SQL ROW STUFF ------------------------
    // SQL_TABLE_LogMessages is a complex object with a lot of
    // construction and destruction time, and a risk of holding a
    // reference to pdb if not released properly.
    //
    // to make vlog() faster (used by LOG* macros everywhere) we'll
    // keep a set of these objects in a pool, and allocate and release
    // from the pool when needed.  we can make the processes use
    // get_log_row() and release_log_row(), but then we have to trust
    // they'll always remember to release.
    //
    // so there's a convenience type (log_row_buf_ref) which does the
    // alloc automatically, and the release automatically, and you
    // just have to let it fall out of scope to release it.
    // ------------------------ SQL ROW STUFF ------------------------

    // we're using msgq_t2t to keep a pool of these, therefore
    // this type must be derived from the t2t base type.
    struct log_row_buf : public ThreadSlinger::thread_slinger_message {
        squirrel_db::SQL_TABLE_LogMessages  row;
        void init(void) { }
        void cleanup(void) { }
    };

    // convenience object, use this one. allocs and releases
    // automatically so you don't have to remember. just let it
    // fall out of scope and it cleans itself up.
    struct log_row_buf_ref {
        log_row_buf * lrb;
        log_row_buf_ref(void) {
            auto hflog = SquirrelLog::instance;
            lrb  = hflog->log_row_pool.alloc(0);
            if (!lrb)
            {
                lrb = new log_row_buf;
                lrb->row.set_db(instance->pdb);
                pxfe_pthread_mutex_lock lock(hflog->all_log_row_bufs_mutex);
                hflog->all_log_row_bufs.push_back(lrb);
            }
        }
        ~log_row_buf_ref(void) {
            SquirrelLog::instance->log_row_pool.release(lrb);
        }
    };

    struct sql_transaction {
        squirrel_db::SQL_TRANSACTION t;
        sql_transaction(void) : t(instance->pdb) {}
        ~sql_transaction(void) { }
    };

    // when the signal handlers detect a signal, this callback
    // function will be called. use it to perform a graceful shutdown
    // of your process.
    //
    // if fatal=false, you can return from your handler, because it was
    // for an externally-input signal (e.g. SIGINT, SIGQUIT) which had
    // nothing to do with the thread running at the time, so the thread
    // running at the time could continue if you wanted it to (e.g. if
    // your callback posts a message to a queue and you want things to
    // go on as normal until that message gets processed by the thread
    // it's supposed to go to, etc).
    //
    // but if fatal=true, DO NOT TRUST very much of your system,
    // because you don't know what's corrupt to have caused the crash!
    // get out as quickly as possible using as few threads as
    // possible, and do not return from the handler unless you want
    // SquirrelLog to do an abort() for you. maybe try to post a message
    // to your main() to cleanup and exit and then sleep for a few seonds
    // to see if it works and then return so SquirrelLog calls abort().
    //
    // note that if we recurse into the signal handler (fault after a
    // fault) this is bad, we do not call the handler and we just abort()
    // as quickly as possible. nothing is safe at that point.
    typedef void (*exit_callback_func_t)(int signal_number, bool fatal);
    static exit_callback_func_t exit_callback_function;

    // every time someone calls LOG_*, this function is called,
    // so that log messages can be printed or sent to other software
    // as required. by default, it points to default_log_callback(),
    // which does fprintf to stderr.
    // this can be changed at any time and takes effect immediately.
    typedef void (*logmsg_callback_func_t)(
        std::shared_ptr<log_row_buf_ref> row);
    static logmsg_callback_func_t logmsg_callback_function;
    static void default_log_callback(
        std::shared_ptr<SquirrelLog::log_row_buf_ref> row);

    // use this function to retrieve and/or delete log rows
    // from the database.
    static void iterate(logmsg_callback_func_t  callback);

    // utilities for logging and time conversion.
    static std::string hexdump_buffer(const uint8_t *buf, int len);
    static double timeval_to_double(const struct timeval &tv);
    static void double_time_to_timeval(struct timeval &tv, double t);
    // format for time conversion: "%Y/%m/%d %H:%M:%S.uuuuuu"
    static const int timestr_len = 30;
    static void timeval_to_timestr(char time_text[timestr_len],
                                   const struct timeval &tv);
    // get the current thread-ID (can you believe that linux
    // does not have a C library function for this?!)
    static int gettid(void);

private:
    static SquirrelLog * instance;
    SquirrelLog(const std::string &dbpath, bool allow_core);
    ~SquirrelLog(void);

    std::string  dbpath;
    bool _ok;
    sqlite3 * pdb;

    // the actual implementation of log() (and all the LOG* macros)
    void vlog(const char* file, int line, bool do_backtrace,
              const char* area, int level,
              const char* format, va_list args)
        __attribute__((__format__ (__printf__, 7, 0)));

    const char *process_name;

    // signal handling. two versions: some signals are crash signals
    // which should show the function name where it occurred, while
    // other signals are externally triggered and it doesn't matter what
    // function was running when it happened. both result in calling
    // the exit_callback_function callback to exit the process.
    static void signal_handler_trace(int sig, siginfo_t *info, void *);
    static void signal_handler_exit(int sig);
    // recursion protection on signal handlers.
    static bool in_signal_handler;

    static int sql_busy(void *arg, int invocations);
    static void init_sql_tables(sqlite3 *pdb,
                                const std::string &table_name,
                                int version_in_file,
                                int version_in_code);
    static void log_sql_upd(void *arg, sqlite3_stmt *stmt);
    static void log_sql_get(void *arg, sqlite3_stmt *stmt);
    static void log_sql_row(void *arg, const std::string &msg);
    static void log_sql_err(void *arg, const std::string &msg);

    pxfe_pthread_mutex all_log_row_bufs_mutex;
    std::vector<log_row_buf*>  all_log_row_bufs;
    // the pool of SQL_TABLE_LogMessages. starts at 0, and
    // grows each time it is needed to grow.
    ThreadSlinger::thread_slinger_pool<log_row_buf> log_row_pool;
};

// normal log entry using printf-style formatting
#define LOG_F(area, level, ...)                                 \
    SquirrelLog::log(                                           \
        __FILE__, __LINE__, false, area, level, __VA_ARGS__)

// log entry using printf-style formatting, with backtrace
#define LOG_FBT(area, level, ...)                               \
    SquirrelLog::log(                                           \
        __FILE__, __LINE__, true, area, level, __VA_ARGS__)

// cout << "stuff" stream-style logging.
#define LOG_S(area, level)                                              \
    SquirrelLog::StreamLogger(__FILE__, __LINE__, false, area, level)

#endif // __SQUIRREL_LOG_H__
