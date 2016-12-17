
#include "childprocessmanager.h"
#include "bufprintf.h"
#include <iostream>
#include <stdlib.h>

class TestHandle : public ChildProcessManager::Handle {
    int inst;
    bool done;
public:
    TestHandle(int _inst) : inst(_inst) {
        done = false;
        if (0) // debug
        {
            Bufprintf<80> bufp;
            bufp.print("constructing testhandle inst %d\n", inst);
            bufp.write(1);
        }
        switch (inst)
        {
        case 1:
            cmd.push_back("cat");
            break;
        case 2:
            cmd.push_back("sh");
            break;
        case 3:
            cmd.push_back("ls");
            cmd.push_back("-l");
            break;
        }
        cmd.push_back(NULL);
    }
    virtual ~TestHandle(void) {
        if (0) // debug
        {
            Bufprintf<80> bufp;
            bufp.print("~TestHandle destructor inst %d\n", inst);
            bufp.write(1);
        }
    }
    /*virtual*/ void handleOutput(const char *buffer, size_t len) {
        std::cout << "data from inst " << inst << std::endl;
        if (write(1, buffer, len) < 0)
            std::cerr << "handleOutput: write failed\n";
    }
    /*virtual*/ void processExited(int status) {
        std::cout << "processExited called for inst " << inst << std::endl;
        done = true;
    }
    bool getDone(void) { return done; }
};

void
usage(void)
{
    printf("usage: childprocessmanager_test [1 | 2]\n");
    exit(1);
    /*NOTREACHED*/
}

int
main(int argc, char ** argv)
{
    if (argc != 2)
        usage();

    int test = atoi(argv[1]);

    // sigpipe is a fucking douchefucker. fuck him.
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;//SA_RESTART;
    sigaction(SIGPIPE, &sa, NULL);

    std::cout << "my pid is " << getpid() << std::endl;

    ChildProcessManager::Manager::instance();
    if (test == 1)
    {
        TestHandle hdl1(1);
        TestHandle hdl2(2);
        hdl1.createChild();
        hdl2.createChild();
        char buf[256];
        int cc;

        while (hdl1.getOpen() || hdl2.getOpen())
        {
            cc = read(0, buf, sizeof(buf));
            if  (cc == 0)
                break;
            hdl1.writeInput(buf, cc);
            hdl2.writeInput(buf, cc);
        }
        if (hdl1.getOpen())
            hdl1.closeChildInput();
        if (hdl2.getOpen())
            hdl2.closeChildInput();
        while (hdl1.getOpen() && hdl2.getOpen())
            usleep(1);
    }
    else if (test == 2)
    {
        int count = 1;
        while (1)
        {
            TestHandle hdl1(3);
            hdl1.createChild();
            while (hdl1.getDone() == false)
                usleep(1);
            printf("job %d is done\n", count++);
            usleep(1000);
        }
    }
    ChildProcessManager::Manager::cleanup();
    return 0;
}
