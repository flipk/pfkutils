
#include "i3_protossl_factory.h"

/********************* i3protoFact *********************/

i3protoFact :: i3protoFact(const i3_options &_opts)
    : opts(_opts)
{
}

i3protoFact :: ~i3protoFact(void)
{
}

ProtoSSL::_ProtoSSLConn *
i3protoFact :: newConnection(void)
{
    return new i3protoConn(opts);
}

/********************* i3protoConn *********************/

i3protoConn :: i3protoConn(const i3_options &_opts)
    : opts(_opts)
{
    if (opts.debug_flag)
        printf("i3protoConn :: i3protoConn\n");
}

i3protoConn :: ~i3protoConn(void)
{
    if (opts.debug_flag)
        printf("i3protoConn :: ~i3protoConn\n");
    msgs->stop();
}

bool
i3protoConn :: messageHandler(const PFK::i3::i3Msg &msg)
{
    if (opts.debug_flag)
        printf("i3protoConn :: messageHandler\n");
    return false;
}

void
i3protoConn :: handleConnect(void)
{
    if (opts.debug_flag)
        printf("i3protoConn :: handleConnect\n");
}
