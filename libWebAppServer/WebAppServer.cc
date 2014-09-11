/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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
