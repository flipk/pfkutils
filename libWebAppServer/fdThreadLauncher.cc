
#include "fdThreadLauncher.h"
#include "myTimeval.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <netdb.h>

using namespace std;

namespace WebAppServer {

fdThreadLauncher :: fdThreadLauncher(void)
{
    fd = -1;
    pollInterval = -1;
    state = INIT;
}

void
fdThreadLauncher :: startFdThread(int _fd, int _pollInterval)
{
    if (fd != -1)
        cerr << "fdThreadLauncher :: startFdThread: already started!" << endl;
    fd = _fd;
    pollInterval = _pollInterval;
    state = STARTING;
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    pthread_t id;
    pthread_create(&id, &attr,
                   &_threadEntry, (void*) this);
    pthread_attr_destroy( &attr );
    int count = 100000;
    while (state == STARTING && count > 0)
    {
        count--;
        usleep(1);
    }
    if (state != RUNNING)
    {
        cerr << "thread failed to start, state : " << state << endl;
        state = DEAD;
    }
}

fdThreadLauncher :: ~fdThreadLauncher(void)
{
    if (state == RUNNING)
        stopFdThread();
}

void
fdThreadLauncher :: setPollInterval(int _pollInterval)
{
    pollInterval = _pollInterval;
    char c = CMD_CHANGEPOLL;
    (void) ::write(cmdFds[1], &c, 1);
}

void
fdThreadLauncher :: stopFdThread(void)
{
    if (state != RUNNING)
        return;
    state = STOPPING;
    char c = CMD_CLOSE;
    (void) ::write(cmdFds[1], &c, 1);
    int count = 100000;
    while (state == STOPPING && count > 0)
    {
        count--;
        usleep(1);
    }
}

//static
void *
fdThreadLauncher :: _threadEntry(void *arg)
{
    fdThreadLauncher * obj = (fdThreadLauncher *) arg;
    pipe(obj->cmdFds);
    obj->state = RUNNING;
    printf("fdThread starting to manage fd %d\n", obj->fd);
    obj->threadEntry();
    printf("fdThread managing fd %d is exiting!\n", obj->fd);
    close(obj->fd);
    if (obj->cmdFds[0] > 0)
        close(obj->cmdFds[0]);
    if (obj->cmdFds[1] > 0)
        close(obj->cmdFds[1]);
    obj->fd = -1;
    obj->cmdFds[0] = -1;
    obj->cmdFds[1] = -1;
    if (obj->state == RUNNING)
        obj->done();
    obj->state = DEAD;
    return NULL;
}

void
fdThreadLauncher :: threadEntry(void)
{
    fd_set rfds, wfds;
    int maxfd;
    int cc;
    int currentPollInterval = -1;
    myTimeval lastPoll;
    myTimeval interval;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    maxfd = cmdFds[0];
    if (fd > maxfd)
        maxfd = fd;
    maxfd++;
    gettimeofday(&lastPoll, NULL);
    while (1)
    {
        FD_SET(cmdFds[0], &rfds);
        bool forRead, forWrite;
        if (doSelect(&forRead,&forWrite) == false)
            break;
        if (forRead)
            FD_SET(fd, &rfds);
        if (forWrite)
            FD_SET(fd, &wfds);
        if (currentPollInterval != pollInterval)
        {
            currentPollInterval = pollInterval;
            if (currentPollInterval > 0)
            {
                interval.tv_sec = currentPollInterval / 1000;
                int ms = currentPollInterval % 1000;
                interval.tv_usec = ms * 1000;
            }
        }
        if (currentPollInterval > 0)
        {
            myTimeval nextPoll, now;
            nextPoll = lastPoll + interval;
            gettimeofday(&now, NULL);
            if (nextPoll > now)
            {
                myTimeval tv = nextPoll - now;
                cc = select(maxfd, &rfds, &wfds, NULL, &tv);
            }
            else
            {
                // force a poll
                cc = 0;
            }
        }
        else
        {
            cc = select(maxfd, &rfds, &wfds, NULL, NULL);
        }
        if (cc < 0)
        {
            printf("select: %s\n", strerror(errno));
            break;
        }
        if (cc == 0)
        {
            if (doPoll() == false)
                break;
            gettimeofday(&lastPoll, NULL);
            continue;
        }
        if (FD_ISSET(fd, &rfds))
            if (handleReadSelect(fd) == false)
                break;
        if (FD_ISSET(fd, &wfds))
            if (handleWriteSelect(fd) == false)
                break;
        if (FD_ISSET(cmdFds[0], &rfds))
        {
            char c;
            if (read(cmdFds[0], &c, 1) != 1)
                break;
            if (c == CMD_CLOSE)
                break;
            if (c == CMD_CHANGEPOLL)
                continue;
        }
    }
}

int
fdThreadLauncher :: acceptConnection(void)
{
    struct sockaddr_in sa;
    socklen_t  salen = sizeof(sa);
    return accept(fd, (struct sockaddr *)&sa, &salen);
}

// static
int
fdThreadLauncher :: makeListeningSocket(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        fprintf(stderr, "socket : %s\n", strerror(errno));
        exit(1);
    }
    int v = 1;
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                (void*) &v, sizeof( v ));
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons((short)port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        fprintf(stderr, "bind : %s\n", strerror(errno));
        exit(1);
    }
    listen(fd, 1);
    return fd;
}

// static
int
fdThreadLauncher :: makeConnectingSocket(uint32_t ip, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        fprintf(stderr, "socket : %s\n", strerror(errno));
        exit(1);
    }
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons((short)port);
    sa.sin_addr.s_addr = htonl(ip);
    int cc = connect(fd, (struct sockaddr *)&sa, sizeof(sa));
    if (cc < 0)
    {
        cerr << "connect : " << strerror(errno) << endl;
        ::close(fd);
        return -1;
    }
    return fd;
}

// static
int
fdThreadLauncher :: makeConnectingSocket(const std::string &host, int port)
{
    uint32_t ip = -1;
    if (!inet_aton(host.c_str(), (in_addr*) &ip))
    {
        struct hostent * he;
        if ((he = gethostbyname(host.c_str())) == NULL)
        {
            cerr << "lookup of host '" << host << "': "
                 << strerror( errno ) << endl;
            return -1;
        }
        memcpy( &ip, he->h_addr, sizeof(ip));
    }
    return makeConnectingSocket(ntohl(ip),port);
}

std::ostream &operator<<(std::ostream &ostr,
                         const fdThreadLauncher::fdThreadState state)
{
    switch (state)
    {
    case fdThreadLauncher::INIT:     ostr << "INIT";     break;
    case fdThreadLauncher::STARTING: ostr << "STARTING"; break;
    case fdThreadLauncher::RUNNING:  ostr << "RUNNING";  break;
    case fdThreadLauncher::STOPPING: ostr << "STOPPING"; break;
    case fdThreadLauncher::DEAD:     ostr << "DEAD";     break;
    default:           ostr << "(" << (int)state << ")"; break;
    }
    return ostr;
}

} // namespace WebAppServer
