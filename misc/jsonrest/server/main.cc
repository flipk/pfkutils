
#include "handlers.h"

int
main(int argc, char ** argv)
{
    myHandlers         hdlrs( xxxxx );
    TestHttpServer     svr(SERVER_PORT);

    hdlrs.register_handlers(svr);

    svr.run();

    return 0;
}

