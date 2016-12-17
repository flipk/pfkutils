
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pty.h>
#include <utmp.h>

#include "config.h"
#include "log.h"

int
bglog_main(int argc, char ** argv)
{
    char * args[MAX_ARGS+1];
    int num_args = 0;
    int ind;

    if (argc == 1)
    {
        printf("usage: bglog ...\n");
        return 1;
    }

    for (ind = 1; ind < argc; ind++)
    {
        if (num_args >= MAX_ARGS)
        {
            printf("too many args, bailing out\n");
            return 1;
        }
        args[num_args++] = argv[ind];
    }
    args[num_args] = NULL;

    for (ind = 0; ind < num_args; ind++)
        printf("arg %d : %s\n", ind, args[ind]);

    int master_fd, slave_fd;
    char ttyname[256];

    if (openpty(&master_fd, &slave_fd, ttyname,
                NULL, NULL) < 0)
    {
        printf("fork failed: %d: %s\n", errno, strerror(errno));
        return 1;
    }
    printf("opened tty : %s\n", ttyname);

    int console_fd = dup(1);
    FILE *console_f = fdopen(console_fd, "w");
    setlinebuf(console_f);
    if (daemon(0,0) < 0)
        fprintf(stderr, "daemon failed!\n");

    pid_t pid = fork();
    if (pid < 0)
    {
        // i am disappoint
        fprintf(console_f,
                "fork failed: %d: %s\n", errno, strerror(errno));
        return 1;
    }
    if (pid == 0)
    {
        // child
        close(master_fd);
        if (login_tty(slave_fd) < 0)
        {
            fprintf(console_f,
                "login_tty failed: %d: %s\n", errno, strerror(errno));
            exit(1);
        }

        execvp(args[0], args);
        fprintf(console_f,
                "execvp failed: %d: %s\n", errno, strerror(errno));

        exit(1);
    }
    // parent

    close(slave_fd);
    (void) unlink(CLEANUP_SCRIPT);
    FILE * cleanup_f = fopen(CLEANUP_SCRIPT, "w");
    fprintf(cleanup_f,
            "#!/bin/sh\n"
            "kill %d\n"
            "done=0\n"
            "while [ $done = 0 ] ; do\n"
            "    result=$( ps ax | awk '{ if ($1 == %d) print \"yes\" }' )\n"
            "    if [ x$result = xyes ] ; then\n"
            "        echo still waiting\n"
            "        sleep 0.5\n"
            "    else\n"
            "        done=1\n"
            "    fi\n"
            "done\n"
            "echo process is dead\n"
            "exit 0\n", pid, pid);
    fclose(cleanup_f);
    log_init(console_f);

    while (1)
    {
        struct timeval tv = { 0, 500000 };
        fd_set rfds;
        char buf[200];

        FD_ZERO(&rfds);
        FD_SET(master_fd,&rfds);
        if (select(master_fd+1, &rfds, NULL,
                   NULL, &tv) == 0)
        {
            log_periodic();
            continue;
        }
        if (FD_ISSET(master_fd, &rfds))
        {
            int cc = read(master_fd,buf,sizeof(buf));
            if (cc < 0)
            {
                if (errno != EIO)
                    fprintf(console_f,
                        "read failed: %d: %s\n", errno, strerror(errno));
            }
            if (cc <= 0)
                break;
            log_data(buf,cc);
        }
    }
    close(master_fd);
    log_finish();
    (void) unlink(CLEANUP_SCRIPT);
    fprintf(console_f, "bglog exiting\n");
    fclose(console_f);

    return 0;
}
