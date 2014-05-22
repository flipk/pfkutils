/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "WebAppServer.h"
#include "WebAppServerInternal.h"
#include "sha1.h"
#include "base64.h"
#include "md5.h"

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

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
