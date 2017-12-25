
#include "libprotossl2.h"

ProtoSSLMsgs :: ProtoSSLMsgs(bool _debugFlag/* = false*/)
    : debugFlag(_debugFlag)
{
    ProtoSSLConnClient * cnt = new ProtoSSLConnClient(this);
    ProtoSSLConnServer * svr = new ProtoSSLConnServer(this);
}

ProtoSSLMsgs :: ~ProtoSSLMsgs(void)
{
}

void
ProtoSSLMsgs :: registerServer(ProtoSSLConnServer * svr)
{
    serverList.add_tail(svr);
    serverHash.add(svr);
}

void
ProtoSSLMsgs :: unregisterServer(ProtoSSLConnServer * svr)
{
    serverList.remove(svr);
    serverHash.remove(svr);
}

void
ProtoSSLMsgs :: registerClient(ProtoSSLConnClient * clnt)
{
    clientList.add_tail(clnt);
    clientHash.add(clnt);
}

void
ProtoSSLMsgs :: unregisterClient(ProtoSSLConnClient * clnt)
{
    clientList.remove(clnt);
    clientHash.remove(clnt);
}
