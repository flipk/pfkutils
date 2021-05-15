
#include "tcptunnel.h"


void
TcpTunnel :: start_a_thread(pthread_t *id,thread_args *args)
{
    pxfe_pthread_attr  attr;
    attr.set_detach(true);
    pthread_create(id, attr(), &threadstarter, (void*) args);
}

//static
void *
TcpTunnel :: threadstarter(void * arg)
{
    thread_args *args = (thread_args *) arg;
    (args->tcptun->*(args->func))(args);
    delete args;
    return NULL;
}
