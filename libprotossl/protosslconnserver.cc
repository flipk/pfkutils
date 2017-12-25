
#include "libprotossl2.h"

ProtoSSLConnServer :: ProtoSSLConnServer(ProtoSSLMsgs * _msgs)
    : msgs(_msgs)
{
    msgs->registerServer(this);
}

ProtoSSLConnServer :: ~ProtoSSLConnServer(void)
{
    msgs->unregisterServer(this);
}
