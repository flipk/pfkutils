
#include "i3_protossl_factory.h"
#include "i3_protossl_conn.h"

i3protoFact :: i3protoFact(const i3_options &_opts,
                i3_loop &_loop)
    : opts(_opts), loop(_loop)
{
}

i3protoFact :: ~i3protoFact(void)
{
}

ProtoSSL::_ProtoSSLConn *
i3protoFact :: newConnection(void)
{
    return new i3protoConn(opts, loop);
}
