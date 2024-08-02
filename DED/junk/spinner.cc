
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/prctl.h>
#include "posix_fe.h"

using namespace std;

class spinnerThread : public pxfe_pthread
{
    int ind;
    pxfe_pipe pipe;
    void settitle(void) {
        char buf[17];
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf)-1,"spin%d",ind);
        prctl(PR_SET_NAME, (unsigned long) buf, 0, 0, 0);
    }
    /*virtual*/ void * entry(void *arg) {
        cerr << ind << " ";
        settitle();
        pxfe_select sel;
        while (1) {
            sel.rfds.zero();
            sel.rfds.set(pipe.readEnd);
            sel.tv.set(0,0);
            int cc = sel.select();
            if (cc < 0)
            {
                char * err = strerror(errno);
                cout << "select error : " << err << endl;
            }
            if (cc > 0)
            {
                if (sel.rfds.is_set(pipe.readEnd))
                {
                    char c;
                    if (read(pipe.readEnd,&c,1) < 0)
                        fprintf(stderr, "spinner:entry: read failed\n");
                    break;
                }
            }
        }
        return NULL;
    }
    /*virtual*/ void send_stop(void) {
        char c = 1;
        if (write(pipe.writeEnd,&c,1) < 0)
            fprintf(stderr, "spinner:stop: write failed\n");
    }
public:
    spinnerThread(void) {
    }
    ~spinnerThread(void) {
    }
    void start(int _ind) { ind = _ind; create(); }
};

int main(int argc, char ** argv)
{
    if (argc != 2)
    {
        cerr << "usage: spinner num_threads" << endl;
        return 1;
    }

    int nth = atoi(argv[1]);

    if (nth < 1 || nth > 1000)
    {
        cerr << "number of threads " << nth << " is a bit ridiculous" << endl;
        return 1;
    }

    cout << "starting " << nth << " threads" << endl;
    spinnerThread spinners[nth];

    int ind;
    for (ind = 0; ind < nth; ind++)
        spinners[ind].start(ind);

    pxfe_select sel;

    while (1)
    {
        sel.rfds.zero();
        sel.rfds.set(0);
        sel.tv.set(1,0);
        if (sel.select() > 0)
            break;
        spinners[ind%nth].stopjoin();
        spinners[ind%nth].start(ind);
        ind++;
    }

    for (ind = 0; ind < nth; ind++)
        spinners[ind].stopjoin();

    return 0;
}
