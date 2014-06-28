
#include "hsmthread.h"

#include <unistd.h>

using namespace HSMThread;

class myTestThread : public Thread
{
    int val;
    /*virtual*/ void entry(void)
    {
        std::cout << "HELLO FROM TEST THREAD " << val << "\n";
    }
    /*virtual*/ void stopReq(void)
    {
        //
    }
public:
    myTestThread(int _val)
        : Thread("MYTHREAD"), val(_val)
    {
        //
    }
    ~myTestThread(void)
    {
        //
    }
};


int
main()
{
    try {
        Thread * t1 = new myTestThread(1);
        Thread * t2 = new myTestThread(2);
        Thread * t3 = new myTestThread(3);
        Thread * t4 = new myTestThread(4);
        Thread * t5 = new myTestThread(5);
        Threads::start();
        t1->join();
        t2->join();
        t3->join();
        t4->join();
        t5->join();
        Threads::cleanup();
    }
    catch (DLL3::ListError le)
    {
        std::cout << le.Format();
    }
    catch (ThreadError te)
    {
        std::cout << te.Format();
    }
    return 0;
}
