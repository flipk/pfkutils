/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "WebAppServer.h"
#include "WebAppServerInternal.h"

namespace WebAppServer {

WebAppConnection :: WebAppConnection(void)
{
    connData = NULL;
}

//virtual
WebAppConnection :: ~WebAppConnection(void)
{
    if (connData)
        delete connData;
}

void
WebAppConnection::sendMessage(const WebAppMessage &m)
{
    connData->sendMessage(m);
}

} // namespace WebAppServer
