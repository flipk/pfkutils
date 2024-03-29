
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <string.h>
#include <fcntl.h>

#include "regex.h"
#include "rules.h"
#include "strip_chars.h"
#include "config_file.h"
#include "process_line.h"

void
usage(void)
{
    fprintf(stderr, "usage:\n"
            "   syslog-postprocess [-f syslogd.ini] syslog.txt\n");
    exit(1);
}

int
postprocess_syslog_main( int argc, char ** argv )
{
    char buf[ MAX_LINE_LEN ];
    int len;
    FILE * in;

    rule_files = rule_files_tail = NULL;
    rules = rules_tail = NULL;

    if (argc == 4)
    {
        if (strcmp(argv[1],"-f")!=0)
            usage();
        parse_config_file(argv[2]);
        argv += 2;
        argc -= 2;
    }
    else
        parse_config_file(DEFAULT_CONFIG_FILE);

    if (argc != 2)
        usage();

    in = fopen(argv[1],"re");
    if (!in)
    {
        fprintf(stderr, "error opening input file: %s\n",
                strerror(errno));
        exit(1);
    }

    while ( fgets(buf, sizeof(buf), in))
    {
        len = strlen(buf);
        len = strip_chars(buf,len);
        process_line( buf, len, buf, len );
    }

    fclose(in);
    close_files();
    return 0;
}

