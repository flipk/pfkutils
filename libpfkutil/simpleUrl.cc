
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iostream>
#include <iomanip>

#include "simpleUrl.h"
#include "simpleRegex.h"

using namespace std;

SimpleUrl :: SimpleUrl(void)
{
    _ok = false;
    addr = (uint32_t) -1;
    port = (uint16_t) -1;
}

SimpleUrl :: SimpleUrl(const std::string &url)
{
    parse(url);
}

SimpleUrl :: ~SimpleUrl(void)
{
}

class UrlRegex : public pxfe_regex<> {
    static const char * patt;
public:
    enum groups {
        PROTO    = 1,
        USERNAME = 3,
        PASSWORD = 5,
        HOSTNAME = 7,
        IPADDR   = 8,
        PORTNUM  = 11,
        PATH     = 12,
        ANCHOR   = 14,
        QUERY    = 16
    };
    UrlRegex(void) : pxfe_regex<>(patt) { }
    ~UrlRegex(void) { }
};

const char * UrlRegex::patt =
"^(ws|wss|http|https)" /*PROTO=1*/
"://"
"(" /*2*/
  "([a-zA-Z][a-zA-Z0-9_-]*)" /*USERNAME=3*/
  "(:" /*4*/
    "([a-zA-Z][a-zA-Z0-9_-]*)" /*PASSWORD=5*/
  ")?"
  "@"
")?"
"(" /*6*/
  "([a-zA-Z][a-zA-Z0-9\\.-]*)" /*HOSTNAME=7*/
"|"
  "([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)" /*IPADDR=8*/
")"
"(" /*9*/
  "(:" /*10*/
    "([0-9]+)" /*PORTNUM=11*/
  ")?"
")"
"(/[a-zA-Z0-9/_-]+)" /*PATH=12*/
"(#" /*13*/
    "(.*)" /*ANCHOR=14*/
")?"
"(\\?" /*15*/
    "(.*)" /*QUERY=16*/
")?"
"$";

bool
SimpleUrl :: parse(const std::string &_url)
{
    UrlRegex r;

    if (!r.ok())
    {
        // this should only happen if you changed the code and
        // introduced a bug!
        cerr << "URL REGEX FAILED TO COMPILE, WHAT DID YOU DO RAY\n";
        exit(1);
    }

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

    if (r.match(UrlRegex::USERNAME))
        username = r.match(url, UrlRegex::USERNAME);
    else
        username = "";

    if (r.match(UrlRegex::PASSWORD))
        password = r.match(url, UrlRegex::PASSWORD);
    else
        password = "";

    if (r.match(UrlRegex::HOSTNAME))
    {
        hostname = r.match(url,UrlRegex::HOSTNAME);

// TODO convert to using getaddrinfo

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

    if (r.match(UrlRegex::ANCHOR))
        anchor = r.match(url,UrlRegex::ANCHOR);
    else
        anchor = "";

    if (r.match(UrlRegex::QUERY))
        query = r.match(url,UrlRegex::QUERY);
    else
        query = "";

    return _ok = true;
}

ostream &operator<<(ostream &str, const SimpleUrl &u)
{
    if (u.ok() == true)
        str << "Url:" << endl
            << " url: " << u.url << endl
            << " protocol: " << u.protocol << endl
            << " username: " << u.username << endl
            << " password: " << u.password << endl
            << " host: " << u.hostname << endl
            << " addr: 0x" << hex << setfill('0') << setw(8) << u.addr << endl
            << " port: " << dec << u.port << endl
            << " path: " << u.path << endl
            << " anchor: " << u.anchor << endl
            << " query: " << u.query << endl;
    else
        str << "Url: (not set)" << endl;
    return str;
}

#ifdef INCLUDE_SIMPLE_URL_TEST

std::string testUrls[] = {
    "https://flipk@pfk.org/one/two",
    "wss://pfk.org/three/four",
    "http://127.0.0.1:1080/five#link",
    "http://username1:password2@hostname3.crap.com/path/path",
    "http://user:pass@127.0.0.1:1080/six?var=val&var=val",
    ""
};

int
main()
{
    SimpleUrl  url;

    int ind = 0;
    while (1)
    {
        const std::string &u = testUrls[ind];
        if (u == "")
            break;
        url.parse(u);
        cout << url;
        ind ++;
    }

    return 0;
}

#endif
