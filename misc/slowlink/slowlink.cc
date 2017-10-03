
#include "LockWait.h"
#include "thread_slinger.h"
#include "pfkposix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BAIL(condition,what) \
    if (condition) { \
        int e = errno; \
        fprintf(stderr, what ": %d:%s\n", e, strerror(e)); \
        return 1; \
    }

//#define PRINTF(x...)
#define PRINTF(x...) printf(x)
#define PRINTF2(x...)
//#define PRINTF2(x...) printf(x)

struct fdBuffer : public ThreadSlinger::thread_slinger_message
{
    fdBuffer(void) { buffer = NULL; }
    ~fdBuffer(void) { if (buffer != NULL) delete[] buffer; }
    static int BUFFERSIZE;
    pfk_timeval stamp; // the time at which this should be sent.
    int length;
    char * buffer;
    void init(void) {
        if (buffer == NULL)
            buffer = new char[BUFFERSIZE];
    }
};

int fdBuffer::BUFFERSIZE = 0; // must be initialized

typedef ThreadSlinger::thread_slinger_pool<fdBuffer> fdBufPool;
typedef ThreadSlinger::thread_slinger_queue<fdBuffer> fdBufQ;

struct connection_config
{
    uint32_t latency;
    int server_fd;
    int client_fd;
    bool reader0_running;
    bool reader1_running;
    bool writer0_running;
    bool writer1_running;
    bool die;
    bool doServer;
    WaitUtil::Semaphore  thread_init_sem;
    WaitUtil::Semaphore  client_tokens;
    WaitUtil::Semaphore  server_tokens;
    fdBufPool ctsPool;
    fdBufPool stcPool;
    fdBufQ ctsQ;
    fdBufQ stcQ;
};

#define TICKS_PER_SECOND 20

void * reader_thread_routine( void * );
void * writer_thread_routine( void * );

int
main(int argc, char ** argv)
{
    if (argc != 6)
    {
        fprintf(stderr,
                "usage: slowlink bytes_per_second latency_in_mS "
                "listen_port remote_ip remote_port\n");
        return 1;
    }

    try {
        connection_config  cfg;

        fdBuffer::BUFFERSIZE = atoi(argv[1]) / TICKS_PER_SECOND;
        cfg.latency = atoi(argv[2]);
        uint16_t listen_port = atoi(argv[3]);
        char * remote_ip = argv[4];
        uint16_t remote_port = atoi(argv[5]);
        int v = 1, listen_fd = -1;
        struct sockaddr_in sa;
        struct in_addr remote_inaddr;
        pthread_t       id;
        pthread_attr_t  pthattr;

        if (!inet_aton(remote_ip, &remote_inaddr))
        {
            fprintf(stderr, "unable to parse ip address '%s'\n", remote_ip);
            return 1;
        }

        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        BAIL(listen_fd < 0, "socket");
        cfg.client_fd = socket(AF_INET, SOCK_STREAM, 0);
        BAIL(cfg.client_fd < 0, "socket");
        v = 1;
        setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR,
                    (void*) &v, sizeof( v ));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(listen_port);
        sa.sin_addr.s_addr = htonl(INADDR_ANY);
        v = bind( listen_fd, (struct sockaddr *) &sa, sizeof(sa));
        BAIL(v < 0, "bind");
        listen( listen_fd, 1 );
        socklen_t salen = sizeof(sa);
        cfg.server_fd = accept( listen_fd, (struct sockaddr *) &sa, &salen );
        BAIL( cfg.server_fd < 0, "accept" );
        close( listen_fd );
        sa.sin_port = htons(remote_port);
        sa.sin_addr = remote_inaddr;
        v = connect(cfg.client_fd, (struct sockaddr *) &sa, sizeof(sa));
        BAIL(v < 0, "connect");
        cfg.thread_init_sem.init(0);
        cfg.ctsPool.add(100);
        cfg.stcPool.add(100);
        cfg.reader0_running = false;
        cfg.reader1_running = false;
        cfg.writer0_running = false;
        cfg.writer1_running = false;
        cfg.die = false;
        pthread_attr_init( &pthattr );
        pthread_attr_setdetachstate( &pthattr, PTHREAD_CREATE_DETACHED );
        cfg.doServer = true;
        pthread_create(&id, &pthattr, reader_thread_routine, &cfg);
        cfg.thread_init_sem.take();
        cfg.doServer = false;
        pthread_create(&id, &pthattr, reader_thread_routine, &cfg);
        cfg.thread_init_sem.take();
        cfg.doServer = true;
        pthread_create(&id, &pthattr, writer_thread_routine, &cfg);
        cfg.thread_init_sem.take();
        cfg.doServer = false;
        pthread_create(&id, &pthattr, writer_thread_routine, &cfg);
        cfg.thread_init_sem.take();
        pthread_attr_destroy( &pthattr );
        while (cfg.reader0_running == true &&
               cfg.reader1_running == true &&
               cfg.writer0_running == true && 
               cfg.writer1_running == true)
        {
            usleep(1000000 / TICKS_PER_SECOND);
            // issue tokens
            cfg.client_tokens.give();
            cfg.server_tokens.give();
            PRINTF2("%d %d %d %d\n",
                    cfg.reader0_running, cfg.reader1_running,
                    cfg.writer0_running, cfg.writer1_running);
        };
        // we got here because one of the connections
        // apparently died. attempt to clean up.
        // note: this part doesn't seem to work worth a damn.
        cfg.die = true;
        if (cfg.client_fd != -1)
            close(cfg.client_fd);
        if (cfg.server_fd != -1)
            close(cfg.server_fd);
        cfg.client_tokens.give();
        cfg.server_tokens.give();
        while (cfg.reader0_running == true ||
               cfg.reader1_running == true ||
               cfg.writer0_running == true || 
               cfg.writer1_running == true)
        {
            usleep(1);
        }
    }
    catch (WaitUtil::LockableError e) {
        fprintf(stderr, "LockableError: %s\n", e.Format().c_str());
    }
    catch (DLL3::ListError e) {
        fprintf(stderr, "DLL3 error: %s\n", e.Format().c_str());
    }
    return 0;
}

void * reader_thread_routine( void * _cfg )
{
    connection_config * cfg = (connection_config *) _cfg;
    bool doServer = cfg->doServer;
    bool *myRunning = doServer ? &cfg->reader0_running : &cfg->reader1_running;
    int myfd = doServer ? cfg->server_fd : cfg->client_fd;
    fdBufPool *myPool = doServer ? &cfg->stcPool : &cfg->ctsPool;
    fdBufQ *myQ = doServer ? &cfg->stcQ : &cfg->ctsQ;
    fdBuffer * buf;
    pfk_timeval latency;

    latency.tv_sec = cfg->latency / 1000; // ms to sec
    latency.tv_usec = (cfg->latency % 1000) * 1000; // ms to us
    *myRunning = true;
    cfg->thread_init_sem.give();
    while (cfg->die == false)
    {
        PRINTF("reader %d trying to alloc\n", doServer);
        buf = myPool->alloc(-1);
        buf->init();
        PRINTF("reader %d got a buffer, attempting read\n", doServer);
        buf->length = read(myfd, buf->buffer, fdBuffer::BUFFERSIZE);
        PRINTF("reader %d got read of %d\n", doServer, buf->length);
        if (buf->length <= 0)
        {
            fprintf(stderr, "got read of %d on fd %d\n", buf->length, myfd);
            break;
        }
        gettimeofday(&buf->stamp, NULL);
        buf->stamp += latency;
        myQ->enqueue(buf);
    }
    *myRunning = false;
    return NULL;
}

void * writer_thread_routine( void * _cfg )
{
    connection_config * cfg = (connection_config *) _cfg;
    bool doServer = cfg->doServer;
    bool *myRunning = doServer ? &cfg->writer0_running : &cfg->writer1_running;
    pfk_timeval now;
    WaitUtil::Semaphore *myTokens =
        doServer ? &cfg->server_tokens : &cfg->client_tokens;
    fdBufQ *myQ = doServer ? &cfg->stcQ : &cfg->ctsQ;
    fdBufPool *myPool = doServer ? &cfg->stcPool : &cfg->ctsPool;
    int myfd = doServer ? cfg->client_fd : cfg->server_fd;
    fdBuffer * buf;

    *myRunning = true;
    cfg->thread_init_sem.give();
    while (cfg->die == false && myTokens->take())
    {
        gettimeofday(&now, NULL);
        buf = myQ->get_head();
        if (buf == NULL)
            // queue empty, nothing to do.
            continue;
        if (buf->stamp > now)
        {
            // packet not old enough, let it age.
            PRINTF2("writer %d now %d.%06d < stamp %d.%06d\n",
                    doServer,
                    (int)now.tv_sec, (int)now.tv_usec,
                    (int)buf->stamp.tv_sec, (int)buf->stamp.tv_usec);
            continue;
        }
        buf = myQ->dequeue();
        PRINTF("writer %d attempting %d\n", doServer, buf->length);
        int cc = write(myfd, buf->buffer, buf->length);
        PRINTF("writer %d attempt %d returns %d\n",
               doServer, buf->length, cc);
        if (cc != buf->length)
        {
            fprintf(stderr, "write attempt %d returns %d on fd %d\n",
                    buf->length, cc, myfd);
            cfg->die = true;
            break;
        }
        myPool->release(buf);
    }
    *myRunning = false;
    return NULL;
}
