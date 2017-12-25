
#include "libprotossl2.h"

ProtoSSLConnClient :: ProtoSSLConnClient(ProtoSSLMsgs * _msgs)
    : msgs(_msgs)
{
    msgs->registerClient(this);
}

ProtoSSLConnClient :: ~ProtoSSLConnClient(void)
{
    msgs->unregisterClient(this);
}
