/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

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

// this is not presently supported for FastCGI -- only for websocket.
const struct sockaddr_in *
WebAppConnection::get_remote_addr(void)
{
    if (connData == NULL)
        return NULL;
    WebAppConnectionDataWebsocket * wacdw = connData->ws();
    if (wacdw == NULL)
        return NULL;
    return wacdw->connBase->get_remote_addr();
}

} // namespace WebAppServer
