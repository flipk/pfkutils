
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "pick.H"

#define SHELL "etg_worker", "etg_worker", NULL

static int
handle_connection(void)
{
    if (!pfk_key_pick(false))
        return 1;
    execlp( SHELL );
    return 3;
}

static int child_count = 0;

static void
sigchld(int s)
{
    int dummy_status;
    pid_t np;
    np = wait( &dummy_status );
    fprintf(stderr,"child pid %d exited with status %d\n", np, dummy_status);
    child_count --;
}

extern "C" int
pfkteld_main()
{
    int v, fd, sock;
    struct sockaddr_in sa;
    struct sigaction sigact;
    pid_t np;

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        fprintf(stderr,"socket: %s\n", strerror(errno));
        return 1;
    }

    v = 1;
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (void*) &v, sizeof( v ));

    sa.sin_family = PF_INET;
    sa.sin_port = htons(23);
    sa.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        fprintf(stderr,"bind: %s\n", strerror(errno));
        return 1;
    }

    listen(fd,1);

    sigact.sa_handler = sigchld;
    sigfillset( &sigact.sa_mask );
    sigact.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sigact, NULL);

    while (1)
    {
        socklen_t slen = sizeof(sa);
        sock = accept(fd, (struct sockaddr *)&sa, &slen);
        if (sock > 0)
        {
            unsigned int addr = ntohl(sa.sin_addr.s_addr);

            fprintf(stderr, "\npfkteld: accepting new connection from "
                    "%d.%d.%d.%d\n\n",
                    (addr >> 24) & 0xFF,(addr >> 16) & 0xFF,
                    (addr >>  8) & 0xFF,(addr >>  0) & 0xFF);
            if (child_count > 10)
            {
                close(sock);
                continue;
            }
            np = fork();
            if (np == 0)
            {
                // child
                dup2(sock,0);
                dup2(sock,1);
                for (int i=3; i < 64; i++)
                    close(i);
                setlinebuf(stdout);
                exit(handle_connection());
            }
            else
            {
                fprintf(stderr,"created child pid %d\n", np);
                child_count ++;
                // in case of parent or error, close our sock.
                close(sock);
            }
        }
    }
}
