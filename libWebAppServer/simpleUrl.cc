
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>

#include "simpleWebSocket.h"
#include "simpleRegex.h"

using namespace std;

namespace SimpleWebSocket {

Url :: Url(void)
{
    _ok = false;
    addr = (uint32_t) -1;
    port = (uint16_t) -1;
}

Url :: Url(const std::string &url)
{
    parse(url);
}

Url :: ~Url(void)
{
}

class UrlRegex : public regex<> {
    static const char * patt;
public:
    enum groups {
        PROTO    = 1, 
        HOSTNAME = 3,
        IPADDR   = 4,
        PORTNUM  = 7,
        PATH     = 8
    };
    UrlRegex(void) : regex<>(patt) { }
    ~UrlRegex(void) { }
};

const char * UrlRegex::patt =
"^(ws|wss|http|https)" /*PROTO=1*/
"://"
"(" /*2*/
  "([a-zA-Z][a-zA-Z0-9\\.-]*)" /*HOSTNAME=3*/
"|"
  "([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)" /*IPADDR=4*/
")"
"(" /*5*/
  "(:" /*6*/
    "([0-9]+)" /*PORTNUM=7*/
  ")?"
")"
"(/[a-zA-Z0-9/_-]+)$"; /*PATH=8*/

bool
Url :: parse(const std::string &_url)
{
    UrlRegex r;

    url = _url;

    if (r.exec(url) == false)
    {
        cerr << "parse url " << url << ": MALFORMED\n";
        return _ok = false;
    }

    protocol = r.match(url,UrlRegex::PROTO);
    if (protocol == "ws" || protocol == "http")
        port = 80;
    else if (protocol == "wss" || protocol == "https")
        port = 443;

    if (r.match(UrlRegex::HOSTNAME))
    {
        hostname = r.match(url,UrlRegex::HOSTNAME);

        struct hostent * he = gethostbyname( hostname.c_str() );
        if (he == NULL)
        {
            cerr << "hostname " << hostname << ": DNS lookup failure\n";
            return _ok = false;
        }
        memcpy( &addr, he->h_addr, he->h_length );
        addr = ntohl(addr);
    }
    if (r.match(UrlRegex::IPADDR))
    {
        hostname = r.match(url,UrlRegex::IPADDR);
        struct sockaddr_in a;
        if ( ! inet_aton(hostname.c_str(), &a.sin_addr))
        {
            cerr << "ip addr " << hostname << ": ip parse error\n";
            return _ok = false;
        }
        addr = ntohl(a.sin_addr.s_addr);
    }
    if (hostname == "")
        return _ok = false;

    if (r.match(UrlRegex::PORTNUM))
    {
        port = atoi(r.match(url,UrlRegex::PORTNUM).c_str());
        if (port == 0)
        {
            cerr << "port " << r.match(url,UrlRegex::PORTNUM)
                 << ": port parse error\n";
            return _ok = false;
        }
    }

    path = r.match(url,UrlRegex::PATH);

    return _ok = true;
}

ostream &operator<<(ostream &str, const Url &u)
{
    if (u.ok() == true)
        str << "Url:" << endl
            << " url: " << u.url << endl
            << " protocol: " << u.protocol << endl
            << " host: " << u.hostname << endl
            << " addr: 0x" << hex << setfill('0') << setw(8) << u.addr << endl
            << " port: " << dec << u.port << endl
            << " path: " << u.path << endl;
    else
        str << "Url: (not set)" << endl;
    return str;
}

};
