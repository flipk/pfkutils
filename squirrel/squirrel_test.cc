
#include "squirrel_log.h"

static void log_callback(std::shared_ptr<SquirrelLog::log_row_buf_ref> row)
{
    squirrel_db::SQL_TABLE_LogMessages *r = &row->lrb->row;
    fprintf(stderr, "%s:%d:%s\n",
            r->file.c_str(), r->line, r->message.c_str());
}

static void dumper(std::shared_ptr<SquirrelLog::log_row_buf_ref> row)
{
    squirrel_db::SQL_TABLE_LogMessages *r = &row->lrb->row;
    fprintf(stderr, "%s:%d:%s\n",
            r->file.c_str(), r->line, r->message.c_str());
    r->delete_rowid();
}

int main()
{
    LOG_F("MAIN", SquirrelLog::INFO, "log before logging initialized!");

    SquirrelLog::logmsg_callback_function = &log_callback;
    if (!SquirrelLog::init("obj/test.db"))
    {
        printf("SquirrelLog failed to init, bye\n");
        return 1;
    }

    SquirrelLog::set_thread_name("main");

    LOG_FBT("MAIN", SquirrelLog::INFO, "we have entered main!");
    LOG_S("MAIN", SquirrelLog::DBG) << "this is a streams test";

    printf("\n\n\n NOW DUMPING LOGS\n");
//    SquirrelLog::verbose_sql = true;
    SquirrelLog::sql_transaction  t;
    t.t.begin();
    SquirrelLog::instance->iterate(&dumper);
    t.t.commit();
//    SquirrelLog::verbose_sql = false;

    SquirrelLog::cleanup();
    return 0;
}
