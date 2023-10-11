
#include "squirrel_log.h"
#include <sys/time.h>
#include <execinfo.h>

SquirrelLog * SquirrelLog::instance = NULL;
bool SquirrelLog::verbose_sql = false;
SquirrelLog::exit_callback_func_t
     SquirrelLog::exit_callback_function = NULL;
SquirrelLog::logmsg_callback_func_t
     SquirrelLog::logmsg_callback_function =
        &SquirrelLog::default_log_callback;
bool SquirrelLog :: in_signal_handler = false;

//static
bool SquirrelLog :: init(const std::string &_dbpath, bool allow_core)
{
    if (instance == NULL)
    {
        SquirrelLog * sl = new SquirrelLog(_dbpath, allow_core);
        if (sl->_ok)
            instance = sl;
        else
            delete sl;
    }
    return (instance != NULL);
}

void SquirrelLog :: cleanup(void)
{
    if (instance)
        delete instance;
    instance = NULL;
}

SquirrelLog :: SquirrelLog(const std::string &_dbpath, bool allow_core)
    : dbpath(_dbpath), _ok(false), pdb(NULL)
{
    if (sqlite3_threadsafe() == 0)
    {
        // if you mess up compile time options, you might disable
        // threadsafe setting, which is mandatory for this code.
        // we'll just error out immediately so you can go fix the
        // makefile. that's better than having random corruptions later.
        fprintf(stderr,   "\n\n\n ERROR : "
                "SQLITE3 compiled with THREADSAFE=0!\n\n");
        exit(1);
    }

    all_log_row_bufs_mutex.init();

    process_name = program_invocation_short_name;

    int sqlret = sqlite3_open(dbpath.c_str(), &pdb);
    sqlite3_busy_handler(pdb, &sql_busy, (void*) this);
    squirrel_db::SQL_TABLE_ALL_TABLES::init_all(
        pdb, &SquirrelLog::init_sql_tables);
    squirrel_db::SQL_TABLE_ALL_TABLES::register_log_funcs(
        &log_sql_upd,
        &log_sql_get,
        &log_sql_row,
        &log_sql_err,
        (void*) this);

    if (!allow_core)
    {
        struct sigaction act;
        act.sa_sigaction = &SquirrelLog::signal_handler_trace;
        sigfillset(&act.sa_mask);
        act.sa_flags = SA_SIGINFO;

        if (getenv("ALLOW_CORE_DUMPS") == NULL)
        {
            sigaction(SIGSEGV, &act, NULL);
            sigaction(SIGABRT, &act, NULL);
            sigaction(SIGFPE,  &act, NULL);
            sigaction(SIGBUS,  &act, NULL);
            sigaction(SIGILL,  &act, NULL);
            sigaction(SIGTRAP, &act, NULL);
        }

        act.sa_handler = &SquirrelLog::signal_handler_exit;
        act.sa_flags = SA_RESTART;

        sigaction(SIGINT,  &act, NULL);
        sigaction(SIGQUIT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
        sigaction(SIGHUP,  &act, NULL);

        act.sa_handler = SIG_IGN;

        sigaction(SIGPIPE, &act, NULL);
    }

    _ok = true;
    instance = this;

    LOG_F("SQUIRREL", INFO, "process %s started", process_name);
}

SquirrelLog :: ~SquirrelLog(void)
{
    struct sigaction sa;

    _ok = false;
    instance = NULL;

    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask); // makes valgrind happy
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGTRAP, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);

    if (pdb)
    {
        for (auto &r : all_log_row_bufs)
            r->row.set_db(NULL);
        int sqlret = sqlite3_close(pdb);
        if (sqlret != SQLITE_OK)
        {
            fprintf(stderr, "FAILURE TO CLOSE SQLITE3 DB\n");
        }
    }
}

//static
void SquirrelLog :: log(const char* file, int line, bool do_backtrace,
                        const char* area, int level,
                        const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (instance)
    {
        instance->vlog(file, line, do_backtrace,
                       area, level, format, args);
    }
    else
    {
        char buf[4096];
        if (format)
        {
            vsnprintf(buf, sizeof(buf), format, args);
            buf[4095] = 0;
        }
        else
            buf[0] = 0;

        fprintf(stderr, "NOSQL: %s:%d: %s\n", file, line, buf);
    }
    va_end(args);
}

void SquirrelLog :: vlog(const char* file, int line, bool do_backtrace,
                         const char* area, int level,
                         const char* format, va_list args)
{
    struct timeval now;
    char time_text[timestr_len];
    char buf[4096];
    char thread_name[16];
    static const int bt_buffer_size = 96;
    void *bt_buffer[bt_buffer_size];
    int bt_size = 0;
    char ** syms;
    int pid, tid;

    gettimeofday(&now, NULL);
    timeval_to_timestr(time_text, now);

    if (format)
    {
        vsnprintf(buf, sizeof(buf), format, args);
        buf[4095] = 0;
    }
    else
        buf[0] = 0;

    if (pthread_getname_np(pthread_self(),
                           thread_name, sizeof(thread_name)) < 0)
    {
        strncpy(thread_name, "<unknown>", sizeof(thread_name));
        thread_name[sizeof(thread_name)-1] = 0;
    }

    pid = (int) getpid();
    tid = gettid();

    // we make this a shared_ptr so that the logmsg_callback
    // can hang onto it for as long as it wants (i.e. sitting
    // in a msgq somewhere waiting for a thread) or not at all
    // (if there is no callback registered).
    auto lrbr = std::make_shared<log_row_buf_ref>();

    lrbr->lrb->row.init();
    lrbr->lrb->row.timestamp = timeval_to_double(now);
    lrbr->lrb->row.time_text = time_text;
    lrbr->lrb->row.pid = pid;
    lrbr->lrb->row.process = process_name;
    lrbr->lrb->row.tid = tid;
    lrbr->lrb->row.thread = thread_name;
    lrbr->lrb->row.file = file;
    lrbr->lrb->row.line = line;
    lrbr->lrb->row.message = buf;
    lrbr->lrb->row.area = area;
    lrbr->lrb->row.level = level;

    if (do_backtrace)
    {
        bt_size = backtrace(bt_buffer, bt_buffer_size);
        syms = backtrace_symbols(bt_buffer, bt_size);

        std::ostringstream str;
        str << "-----------------BACKTRACE-----------------\n";
        for (int ind = 0; ind < bt_size; ind++)
            str << syms[ind] << "\n";
        str << "-------------------------------------------\n";
        lrbr->lrb->row.backtrace = str.str();
    }

    if (!lrbr->lrb->row.insert())
    {
        fprintf(stderr, "ERROR FAILURE TO INSERT LOG MESSAGE\n");
    }

    if (logmsg_callback_function)
        logmsg_callback_function(lrbr);
}

//static
void SquirrelLog :: default_log_callback(
    std::shared_ptr<SquirrelLog::log_row_buf_ref> row)
{
    squirrel_db::SQL_TABLE_LogMessages *r = &row->lrb->row;
    fprintf(stderr, "%s:%d:%d:%d:%s:%d:%s\n",
            r->area.c_str(), r->level, r->pid, r->tid,
            r->file.c_str(), r->line, r->message.c_str());
}

//static
void SquirrelLog :: iterate(logmsg_callback_func_t  callback)
{
    auto lrbr = std::make_shared<log_row_buf_ref>();

    if (lrbr->lrb->row.get_all())
    {
        do {
            callback(lrbr);
        } while (lrbr->lrb->row.get_next());
    }
}
