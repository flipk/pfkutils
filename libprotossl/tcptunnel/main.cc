
#include "tcptunnel.h"

int
main(int argc, char ** argv)
{
    TcpTunnel  tt(argc, argv);

    if (tt.ok() == false)
    {
        tt.usage();
        return 1;
    }

    return tt.main();
}
