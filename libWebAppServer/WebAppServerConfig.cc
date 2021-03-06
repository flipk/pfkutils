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
                                 int pollInterval /* = -1 */,
                                 int msgTimeout /* = 0 */)
{
    records.push_back(
        new WebAppServerConfigRecord(
            APP_TYPE_WEBSOCKET,
            port, route, cb, pollInterval, msgTimeout));
}

void
WebAppServerConfig::addWebsocket(int port, const std::string route,
                                 WebAppConnectionCallback *cb,
                                 const std::string ipaddr,
                                 int pollInterval /* = -1 */,
                                 int msgTimeout /* = 0 */)
{
    records.push_back(
        new WebAppServerConfigRecord(
            APP_TYPE_WEBSOCKET,
            port, route, cb, ipaddr, pollInterval, msgTimeout));
}


void
WebAppServerConfig::addFastCGI(int port, const string route,
                               WebAppConnectionCallback *cb,
                               int pollInterval /* = -1 */,
                               int msgTimeout /* = 0 */)
{
    records.push_back(
        new WebAppServerFastCGIConfigRecord(
            APP_TYPE_FASTCGI,
            port, route, cb, pollInterval, msgTimeout));
}

} // namespace WebAppServer
