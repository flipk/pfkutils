
#include "squirrel_log.h"

static void my_exit_callback(int sig, bool fatal)
{
    fprintf(stderr, "TEST EXIT CALLBACK CALLED sig=%d\n", sig);
    if (fatal)
        fprintf(stderr, "FATAL\n");
    else
        fprintf(stderr, "NON FATAL\n");
}

static void log_callback(std::shared_ptr<SquirrelLog::log_row_buf_ref> row)
{
    squirrel_db::SQL_TABLE_LogMessages *r = &row->lrb->row;
    fprintf(stderr, "%s:%d:%s\n",
            r->file.c_str(), r->line, r->message.c_str());
    if (r->backtrace.size() > 0)
        fprintf(stderr, "%s\n", r->backtrace.c_str());
}

static void dumper(std::shared_ptr<SquirrelLog::log_row_buf_ref> row)
{
    squirrel_db::SQL_TABLE_LogMessages *r = &row->lrb->row;
    fprintf(stderr, "%s:%d:%s\n",
            r->file.c_str(), r->line, r->message.c_str());
//    r->delete_rowid();
}

int main()
{
    LOG_F("MAIN", SquirrelLog::INFO, "log before logging initialized!");

    SquirrelLog::exit_callback_function = &my_exit_callback;
    SquirrelLog::logmsg_callback_function = &log_callback;
    if (!SquirrelLog::init("obj/test.db"))
    {
        printf("SquirrelLog failed to init, bye\n");
        return 1;
    }

    SquirrelLog::set_thread_name("main");

    LOG_FBT("MAIN", SquirrelLog::INFO, "we have entered main!");
    LOG_S("MAIN", SquirrelLog::DBG) << "this is a streams test";

#if CRASH
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
    {
        char * ptr = (char*) 3;
#pragma message "CRASH enabled:  this file will segfault on purpose"
        *ptr = 4;
    }
#pragma GCC diagnostic pop
#endif

    printf("\n\n\n NOW DUMPING LOGS\n");
//    SquirrelLog::verbose_sql = true;
    SquirrelLog::sql_transaction  t;
    t.t.begin();
    SquirrelLog::iterate(&dumper);
    t.t.commit();
//    SquirrelLog::verbose_sql = false;

    SquirrelLog::cleanup();
    return 0;
}
