#if 0
set -e -x
incs=-Iproj/pfkutils/libpfkutil
libs=-lpthread
g++ $incs mutexfork.cc -o mutexfork $libs
./mutexfork
exit 0
    ;
#endif

// this program demonstrates a nasty bug you can encounter when
// you fork from a multithreaded application. the child cannot
// use mutexes unless the parent guarantees it is okay beforehand.

#include "pfkposix.h"

pfk_pthread_mutex   mtx;

class thr1 : public pfk_pthread
{
    /*virtual*/ void * entry(void) {
        printf("thread one in parent\n");
        sleep(1);
        // thread2 should have locked mutex by now
        pid_t p = fork();
        if (p == 0)
        {
            printf("thread one in child a\n");
            sleep(1);
            // the child will deadlock here, forever!
            pfk_pthread_mutex_lock   lck(mtx);
            printf("thread one in child b\n");
            // child
            _exit(0);
        }
        // parent
        sleep(1);
    }
    /*virtual*/ void send_stop(void) { /*nothing*/ }
public:
    thr1(void) { }
    ~thr1(void) { }
};

class thr2 : public pfk_pthread
{
    /*virtual*/ void * entry(void) {
        pfk_pthread_mutex_lock   lck(mtx);
        printf("thread two\n");
        sleep(10);
    }
    /*virtual*/ void send_stop(void) { /*nothing*/ }
public:
    thr2(void) { }
    ~thr2(void) { }
};

int
main()
{
    thr1  one;
    thr2  two;

    mtx.init();

    one.create();
    two.create();
    one.stopjoin();
    two.stopjoin();

    return 0;
}
