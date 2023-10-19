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
#include "serverPorts.h"

#include <stdlib.h>

using namespace std;

namespace WebAppServer {

std::ostream &operator<<(std::ostream &ostr, const WebAppType type)
{
    if (type == APP_TYPE_WEBSOCKET)
        ostr << "WEBSOCKET";
    else if (type == APP_TYPE_FASTCGI)
        ostr << "FASTCGI";
    else
        ostr << "UNKNOWN(" << (int) type << ")";
    return ostr;
}
std::ostream &operator<<(std::ostream &ostr,
                         const WebAppServerConfigRecord &cr)
{
    ostr << "wascr"
         << " type:" << cr.type
         << " port:" << cr.port
         << " route:" << cr.route;
    if (cr.type == APP_TYPE_FASTCGI)
    {
        const WebAppServerFastCGIConfigRecord * fcgicr =
            dynamic_cast<const WebAppServerFastCGIConfigRecord*>(&cr);
        if (fcgicr == NULL)
        {
            cerr << "ERROR type is cgi but object isn't of type cgi!" << endl;
            exit(1);
        }
    }
    return ostr;
}

WebAppServerServer :: WebAppServerServer(void)
{
    ports = NULL;
}

WebAppServerServer :: ~WebAppServerServer(void)
{
    if (ports)
        delete ports;
}

bool
WebAppServerServer :: start(const WebAppServerConfig *_config)
{
    serverPort::ConfigRecListIterC_t cit;
    serverPorts::portIter_t pit;

    config = _config;

    if (ports)
        delete ports;
    ports = new serverPorts;

    for (cit = config->getBeginC(); cit != config->getEndC(); cit++)
    {
        ports->addConfigRec(*cit);
    }

    for (pit = ports->portMap.begin(); 
         pit != ports->portMap.end(); pit++)
    {
        pit->second->startThread();
    }
    return true;
}

void
WebAppServerServer :: stop(void)
{
    if (ports)
    {
        serverPorts::portIter_t pit;
        for (pit = ports->portMap.begin(); 
             pit != ports->portMap.end(); pit++)
        {
            pit->second->stopThread();
        }
    }
}

} // namespace WebAppServer
