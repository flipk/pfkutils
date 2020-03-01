#if 0
set -e -x
g++ test_ns.cc -o test_ns -lpthread
sudo ./test_ns
exit 0;
#endif

#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sstream>
#include <sys/mman.h>
#include <grp.h>

#define STACK_SIZE (1024 * 1024)

// you can share conditions and mutexes across a clone,
// but only if they're mmap'd in MAP_SHARED memory space
// and marked as PSHARED
struct syncer {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    volatile bool waiting;
};

static int childFunc(void *arg)
{
    struct syncer * s = (struct syncer *) arg;

//    printf("childFunc %d : i am about to exec bash\n", getpid());

    struct timespec expire;
    clock_gettime( CLOCK_REALTIME, &expire );
    expire.tv_sec += 5;
//    printf("childFunc %d : going to sleep on condition\n", getpid());
    pthread_mutex_lock(&s->mutex);
    s->waiting = true;
    pthread_cond_timedwait(&s->cond, &s->mutex, &expire);
    pthread_mutex_unlock(&s->mutex);
//    printf("childFunc %d : condition done\n", getpid());

    setdomainname("NONAME", 6);
    sethostname("NONAME", 6);

    system("bash ./test_ns_inside_ns_jail_setup.sh");
    chroot("/tmp/test_ns");
    chdir("/home/user");
    setenv("USER", "user", 1);
    setenv("LOGNAME", "user", 1);
    setenv("HOME", "/home/user", 1);
    setenv("PS1", " % ", 1);

    // revoke ALL permissions.
    gid_t  gid = 9001;
    setgroups(1, &gid);
    setgid(9001); // set gid before uid because you cant set gid after
    setuid(9001);

    execl("/bin/bash", "bash", NULL);
    int e = errno;
    char * err = strerror(e);
    printf("exec bash failed: %d: %s\n", e, err);
    return 0;
}

int
main()
{
    pid_t pid;

    struct syncer * s = (struct syncer *)mmap(
        NULL, sizeof(struct syncer),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANON,
        /*fd*/-1, /*offset*/0);

    pthread_mutexattr_t            mattr;
    pthread_mutexattr_init       (&mattr);
    pthread_mutexattr_setpshared (&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&s->mutex, &mattr);
    pthread_mutexattr_destroy    (&mattr);

    pthread_condattr_t           cattr;
    pthread_condattr_init      (&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&s->cond, &cattr);
    pthread_condattr_destroy   (&cattr);

    char * stack = (char*) malloc(STACK_SIZE);
    char * stackTop = stack + STACK_SIZE;


    // all the scripts use this
    setenv("newroot","/tmp/test_ns",1);

    s->waiting = false;
    pid = clone(&childFunc, stackTop,
// CLONE_NEWUSER is not supported on every kernel,
// plus i don't know how to use it.
                CLONE_NEWUTS |
                CLONE_NEWPID | CLONE_NEWNET | CLONE_NEWNS | SIGCHLD,
                /*arg*/ (void*) s);

    if (pid < 0)
    {
        int e = errno;
        char * err = strerror(e);
        printf("clone returned %d (%s)\n", e, err);
        return 1;
    }

    bool child_waiting = false;
    do {
        pthread_mutex_lock(&s->mutex);
        if (s->waiting)
            child_waiting = true;
        pthread_mutex_unlock(&s->mutex);
        if (!child_waiting)
        {
            // child hasn't entered cond_wait yet, wait for it.
//            printf("main %d : bonk\n", getpid());
            usleep(1);
        }
    } while (!child_waiting);

    printf("main %d : child pid %d created\n", getpid(), pid);

    // can't create the virtual network until we know the child pid.
    std::ostringstream   cmd;
    cmd << "bash ./test_ns_outside_ns_jail_setup.sh "
        << (int) pid;
    system(cmd.str().c_str());

    // wake up the new child so he can see his new network.
    pthread_cond_broadcast(&s->cond);

//    printf("main %d : entering waitpid\n", getpid());
    pid_t r = waitpid(-1, NULL, 0);
    if ( r < 0)
    {
        int e = errno;
        char * err = strerror(e);
        printf("main %d : waitpid returned %d (%d: %s)\n",
               getpid(), r, e, err);
    }
    else
        printf("main %d : waitpid returned %d\n", getpid(), r);

    system("bash ./test_ns_outside_shutdown.sh");

    return 0;
}
