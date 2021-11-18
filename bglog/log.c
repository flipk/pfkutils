
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "log.h"

static FILE* console_f;

static FILE *logfile;
static char logfile_names[MAX_LOG_FILES][256];
static int current_logfile;
static int current_logsize;

static void
log_open(void)
{
    char cmd[512];
    char * fn;
    int prev_logfile;

    if (current_logfile != 0)
        prev_logfile = current_logfile - 1;
    else
        prev_logfile = MAX_LOG_FILES-1;

    fn = logfile_names[prev_logfile];
    if (fn[0] != 0)
    {
        snprintf(cmd, sizeof(cmd),
                 "nice bzip2 %s &", fn);
        if (system(cmd) < 0)
            fprintf(stderr, "system(%s) failed\n", cmd);
        fprintf(console_f, "bzipped %s\n", fn);
    }

    fn = logfile_names[current_logfile];
    if (fn[0] != 0)
    {
        snprintf(cmd, sizeof(cmd),
                 "%s.bz2", fn);
        (void) unlink( cmd );
        fprintf(console_f, "removed %s\n", cmd);
    }
    if (logfile)
        fclose(logfile);

    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    struct tm tm_now;
    localtime_r(&tv_now.tv_sec, &tm_now);
    char datestr[40];
    strftime(datestr, sizeof(datestr),
             "%Y-%m%d-%H%M%S", &tm_now);

    snprintf(fn, 256, LOGFILE_BASE "-%s-%03u",
             datestr, (unsigned int) tv_now.tv_usec / 1000);

    logfile = fopen(fn, "we");
    if (logfile == NULL)
        fprintf(console_f, "open '%s' : %d: %s\n",
                fn, errno, strerror(errno));

    fprintf(console_f, "opened %s\n", fn);

    current_logsize = 0;
    if (++current_logfile == MAX_LOG_FILES)
        current_logfile = 0;
}

void
log_init(FILE *_console_f)
{
    console_f = _console_f;
    current_logfile = 0;
    current_logsize = 0;
    memset(logfile_names, 0, sizeof(logfile_names));
    if (system("rm -f " LOGFILE_BASE "*") < 0)
        fprintf(stderr, "system(rm -rf logfilebase) failed\n");
    log_open();
}

void
log_data(const char *buf, int len)
{
//    fwrite(buf, len, 1, console_f);
    if (logfile)
    {
        fwrite(buf, len, 1, logfile);
        current_logsize += len;
        if (current_logsize > MAX_FILE_SIZE)
        {
            log_open();
        }
    }
}

void
log_periodic(void)
{
    if (logfile)
        fflush(logfile);
}

void
log_finish(void)
{
    fclose(logfile);
    logfile = NULL;
}
