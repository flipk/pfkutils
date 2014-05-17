
#include "WebAppServer.h"
#include "WebAppServerInternal.h"

using namespace std;

namespace WebAppServer {

WebAppServerConfig::WebAppServerConfig(void)
{
}

WebAppServerConfig::~WebAppServerConfig(void)
{
    clear();
}

void
WebAppServerConfig::clear(void)
{
    serverPort::ConfigRecListIter_t it;
    for (it = records.begin(); it != records.end(); )
    {
        WebAppServerConfigRecord *cr = *it;
        delete cr;
        it = records.erase(it);
    }
}

void
WebAppServerConfig::addWebsocket(int port, const std::string route,
                                 WebAppConnectionCallback *cb,
                                 int pollInterval /* = -1 */)
{
    records.push_back(
        new WebAppServerConfigRecord(
            APP_TYPE_WEBSOCKET,
            port, route, cb, pollInterval));
}

void
WebAppServerConfig::addFastCGI(int port, const string route,
                               WebAppConnectionCallback *cb,
                               int pollInterval /* = -1 */)
{
    records.push_back(
        new WebAppServerFastCGIConfigRecord(
            APP_TYPE_FASTCGI,
            port, route, cb, pollInterval));
}

} // namespace WebAppServer
