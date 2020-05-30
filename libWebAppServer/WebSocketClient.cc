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

#include "pfkutils_config.h"
#include "WebSocketClient.h"
#include <errno.h>
#include <sys/types.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include "mbedtls/sha1.h"
#include "base64.h"

#define VERBOSE 0

using namespace std;
using namespace WebAppServer;

namespace WebAppClient {

#define __WSERR(e) do { \
        WSClientError  wsce(WSClientError::e);  \
    } while(0)

const std::string WSClientError::errStrings[__NUMERRS] = {
    "URL malformed",
    "URL path malformed",
    "PROXY malformed",
    "PROXY hostname not resolved",
    "PROXY IP parse failure",
    "URL hostname not resolved",
    "URL IP parse failure",
    "could not create socket",
    "Server Connection Refused",
    "Trying to send message while not connected"
};

//virtual
const std::string
WSClientError::_Format(void) const
{
    std::string ret = "WebSocketClient ERROR: ";
    ret += errStrings[err];
    ret += " at:\n";
    return ret;
}

// proxy format should be "hostname:port" or "a.b.c.d:port".
//   note port is not required for proxy, assumed to be 1080 if absent.

enum proxy_regex_groups {
    PROXY_GROUP_HOSTNAME = 2,
    PROXY_GROUP_IPADDR   = 4,
    PROXY_GROUP_PORTNUM  = 7
};
static const char * proxyRegexPattern =
"^((([a-zA-Z][a-zA-Z0-9]*\\.)+[a-zA-Z0-9]+)|([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+))((:([0-9]+))?)$";

// url format should be one of:
//    "ws://host/path"
//    "ws://a.b.c.d/path"
//    "ws://host:port/path"
//    "ws://a.b.c.d:port/path"
// port is not required for url; assumed to be 80 if not specified

enum url_regex_groups {
    URL_GROUP_HOSTNAME = 2,
    URL_GROUP_IPADDR   = 4,
    URL_GROUP_PORTNUM  = 7,
    URL_GROUP_PATH     = 8
};
static const char * urlRegexPattern =
"^ws://((([a-zA-Z][a-zA-Z0-9]*\\.)+[a-zA-Z0-9]+)|([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+))((:([0-9]+))?)(/[a-zA-Z0-9/]+)$";

#define MAX_MATCHES 10

WebSocketClient :: WebSocketClient(const std::string &url)
    : readbuf(MAX_READBUF), bProxyWsWithConnect(false)
{
    init_common("",url);
}

WebSocketClient :: WebSocketClient(const std::string &proxy,
                                   const std::string &url,
                                   bool _proxyWsWithConnect)
    : readbuf(MAX_READBUF), bProxyWsWithConnect(_proxyWsWithConnect)
{
    init_common(proxy,url);
}

void
WebSocketClient :: init_common(const string &proxy,
                               const string &url)
{
    regmatch_t   matches[MAX_MATCHES];
    struct sockaddr_in sa;

    finished = false;
    proxyPort = 1080; //assume
    urlPort = 80; //assume
    bUserConnCallback = false;

#define MATCH(n) (matches[(n)].rm_so != -1)
#define MATCHSTR(str,n) \
    str.substr(matches[(n)].rm_so, \
               matches[(n)].rm_eo - matches[(n)].rm_so)

    if (proxy == "")
        bProxy = false;
    else
    {
        regex_t   proxyReg;
        regcomp(&proxyReg, proxyRegexPattern, REG_EXTENDED);
        if (regexec(&proxyReg, proxy.c_str(), MAX_MATCHES, matches, 0) != 0)
        {
            __WSERR(ERR_PROXY_MALFORMED);
            return;
        }
        if (MATCH(PROXY_GROUP_HOSTNAME))
            proxyHost = MATCHSTR(proxy,PROXY_GROUP_HOSTNAME);
        if (MATCH(PROXY_GROUP_IPADDR))
            proxyIp = MATCHSTR(proxy,PROXY_GROUP_IPADDR);
        if (MATCH(PROXY_GROUP_PORTNUM))
            proxyPort = atoi(MATCHSTR(proxy,PROXY_GROUP_PORTNUM).c_str());
        regfree(&proxyReg);

        if (proxyHost.length() > 0)
            cout << "proxyHost : " << proxyHost <<  endl;
        else
            cout << "proxyIp : " << proxyIp <<  endl;
        cout << "proxyPort : " << proxyPort << endl;

        bProxy = true;
    }

    regex_t  urlReg;
    regcomp(&urlReg, urlRegexPattern, REG_EXTENDED);
    if (regexec(&urlReg, url.c_str(), MAX_MATCHES, matches, 0) != 0)
    {
        __WSERR(ERR_URL_MALFORMED);
        return;
    }
    if (MATCH(URL_GROUP_HOSTNAME))
        urlHost = MATCHSTR(url,URL_GROUP_HOSTNAME);
    if (MATCH(URL_GROUP_IPADDR))
        urlIp = MATCHSTR(url,URL_GROUP_IPADDR);
    if (MATCH(URL_GROUP_PORTNUM))
        urlPort = atoi(MATCHSTR(url,URL_GROUP_PORTNUM).c_str());
    if (MATCH(URL_GROUP_PATH))
        urlPath = MATCHSTR(url,URL_GROUP_PATH);
    else
    {
        __WSERR(ERR_URL_PATH_MALFORMED);
        return;
    }
    regfree(&urlReg);

    if (urlHost.length() > 0)
        cout << "urlHost : " << urlHost << endl;
    else
        cout << "urlIp : " << urlIp << endl;
    cout << "urlPort : " << urlPort << endl;
    cout << "urlPath : " << urlPath << endl;

    struct in_addr destAddr;
    int destPort = 80;
    if (proxyHost.length() > 0)
    {
        struct hostent * he = gethostbyname( proxyHost.c_str() );
        if (he == NULL)
        {
            __WSERR(ERR_PROXY_HOSTNAME);
            return;
        }
        memcpy( &destAddr.s_addr, he->h_addr, he->h_length );
        destPort = proxyPort;
    }
    else if (proxyIp.length() > 0)
    {
        if ( ! inet_aton(proxyIp.c_str(), &destAddr))
        {
            __WSERR(ERR_PROXY_IP);
            return;
        }
        destPort = proxyPort;
    }
    else if (urlHost.length() > 0)
    {
        struct hostent * he = gethostbyname( urlHost.c_str() );
        if (he == NULL)
        {
            __WSERR(ERR_URL_HOSTNAME);
            return;
        }
        memcpy( &destAddr.s_addr, he->h_addr, he->h_length );
        destPort = urlPort;
    }
    else if (urlIp.length() > 0)
    {
        if ( ! inet_aton(urlIp.c_str(), &destAddr))
        {
            __WSERR(ERR_URL_IP);
            return;
        }
        destPort = urlPort;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(destPort);
    sa.sin_addr = destAddr;

    newfd = socket(AF_INET, SOCK_STREAM, 0);
    if (newfd < 0)
    {
        __WSERR(ERR_SOCKET);
        return;
    }

    if (connect(newfd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        close(newfd);
        __WSERR(ERR_CONNREFUSED);
        return;
    }

    ostringstream hdrs;

    if (bProxy && bProxyWsWithConnect)
    {
        generateProxyHeaders(hdrs);
        state = STATE_PROXYRESP;
    }
    else
    {
        generateWsHeaders(hdrs);
        state = STATE_HEADERS;
    }

    const string &hdrstr = hdrs.str();
    int writeLen = hdrstr.length();
    int cc = ::write(newfd, hdrstr.c_str(), writeLen);
    if (cc != writeLen)
    {
        fprintf(stderr, "WebSocketClient :: init_common: "
                "write %d returned %d (err %d: %s)\n",
                writeLen, cc, errno, strerror(errno));
    }

    got_flags = GOT_NONE;
}

WebSocketClient :: ~WebSocketClient(void)
{
    stopFdThread();
}

void
WebSocketClient :: startClient(void)
{
    startFdThread( newfd );
}

void
WebSocketClient :: generateProxyHeaders(ostringstream &hdrs)
{
    hdrs << "CONNECT " << hostForConn()
         << ":" << urlPort << " HTTP/1.1\r\n"
         << "Host: " << hostForConn() << "\r\n\r\n";

    if (VERBOSE)
        cout << hdrs.str();
}

static const char *wskeychars =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

void
WebSocketClient :: generateWsHeaders(ostringstream &hdrs)
{
    secWebsocketKey.clear();

    int ctr;
    for (ctr = 0; ctr < 22; ctr++)
    {
        int c = random() % 52;
        secWebsocketKey += wskeychars[c];
    }
    secWebsocketKey += "==";

    if (bProxy && !bProxyWsWithConnect)
        hdrs << "GET ws://" << hostForConn()
             << ":" << urlPort
             << urlPath << " HTTP/1.1\r\n";
    else
        hdrs << "GET " << urlPath << " HTTP/1.1\r\n";

    hdrs << "Host: " << hostForConn() << "\r\n"
         << "Upgrade: websocket\r\n"
         << "Connection: Upgrade\r\n"
         << "Origin: http://" << urlHost << "\r\n"
         << "Pragma: no-cache\r\n"
         << "Cache-Control: no-cache\r\n"
         << "Sec-WebSocket-Key: " << secWebsocketKey << "\r\n"
         << "Sec-WebSocket-Version: 13\r\n"
         << "User-Agent: CrapolaFuxors/0.1\r\n\r\n";

    if (VERBOSE)
        cout << hdrs.str();

    // calc secWebsocketKeyResponse

    secWebsocketKey += websocket_guid;

#define SHA1HashSize 20
    uint8_t digest[SHA1HashSize];

    MBEDTLS_SHA1( (const unsigned char *) secWebsocketKey.c_str(),
                  secWebsocketKey.size(),
                  digest );

    uint8_t digest_b64[128];

    memset(digest_b64,  0, sizeof(digest_b64));
    int i, o;
    for (i = 0, o = 0; i < SHA1HashSize; i += 3, o += 4)
    {
        int len = SHA1HashSize - i;
        if (len > 3)
            len = 3;
        b64_encode_quantum(digest + i, len, digest_b64 + o);
    }

    secWebsocketKeyResponse = (char*) digest_b64;
}

/*virtual*/
bool
WebSocketClient :: doSelect(bool *forRead, bool *forWrite)
{
    *forRead = !finished;
    *forWrite = false;
    return true;
}

/*virtual*/
bool
WebSocketClient :: handleReadSelect(int fd)
{
    if (finished)
        return false;

    if (readbuf.remaining() == 0)
    {
        cerr << "read : buffer is full!" << endl;
        return false;
    }

    int cc = readbuf.readFd(fd);
    if (cc < 0)
        cerr << "read : " << strerror(errno) << endl;
    if (cc == 0)
    {
        handle_data();
        cerr << "read: end of data stream" << endl;
    }
    if (cc <= 0)
    {
        return false;
    }

    return handle_data();
}

bool
WebSocketClient :: handle_data(void)
{
    while (state == STATE_PROXYRESP || state == STATE_HEADERS)
    {
        int newline_pos = readbuf.find("\r\n");
        if (newline_pos == CircularReader::npos)
            // not enough present to find a line.
            break;

        const CircularReaderSubstr &hdr = readbuf.substr(0,newline_pos);

        if (VERBOSE)
            cout << "got : " << hdr << endl;

        if (state == STATE_PROXYRESP)
        {
            if (handle_proxyresp(hdr) == false)
                return false;
        }
        else if (state == STATE_HEADERS)
        {
            if (handle_wsheader(hdr) == false)
                return false;
        }

        readbuf.erase0(newline_pos+2);
    }
    if (state == STATE_CONNECTED)
        if (handle_message() == false)
            return false;
    return true;
}

bool
WebSocketClient :: handle_proxyresp(
    const CircularReaderSubstr &hdr)
{
    if (hdr.size() > 0)
    {
        if (hdr.compare(0, "HTTP/"))
        {
            // look for:
            // HTTP/1.1 200 Connection established
            int spacepos = hdr.find(" ");
            if (spacepos == CircularReader::npos)
                return false;
            int nspacepos = hdr.find(" ",spacepos+1);
            if (nspacepos == CircularReader::npos)
                return false;
            int val = atoi(hdr.toString(spacepos+1,
                                        nspacepos-spacepos-1).c_str());
            if (val != 200)
                return false;
        }
        else
        {
            int colonPos = hdr.find(": ");
            if (colonPos == CircularReader::npos)
                return false;
        }
    }
    else
    {
        // mime headers complete
        ostringstream hdrs;
        generateWsHeaders(hdrs);
        state = STATE_HEADERS;
        const string &hdrstr = hdrs.str();
        int writeLen = hdrstr.length();
        int cc = ::write(fd, hdrstr.c_str(), writeLen);
        if (cc != writeLen)
        {
            fprintf(stderr, "WebSocketClient :: handle_proxyresp: "
                    "write %d returned %d (err %d: %s)\n",
                    writeLen, cc, errno, strerror(errno));
        }
    }
    return true;
}

bool
WebSocketClient :: handle_wsheader(const CircularReaderSubstr &hdr)
{
    if (hdr.size() > 0)
    {
        // look for :
        // HTTP/1.1 101 Switching Protocols
        // Connection: upgrade
        // Upgrade: websocket
        // Sec-WebSocket-Accept: tMX7VKvznJvARBQRBiBavqHP5B8=

        if (hdr.compare(0, "HTTP/"))
        {
            int spacepos = hdr.find(" ");
            if (spacepos == CircularReader::npos)
                return false;
            int nspacepos = hdr.find(" ",spacepos+1);
            if (nspacepos == CircularReader::npos)
                return false;
            int val = atoi(hdr.toString(spacepos+1,
                                        nspacepos-spacepos-1).c_str());
            if (val != 101)
                return false;
            got_flags |= GOT_SWITCHING;
        }
        else
        {
            int colonPos = hdr.find(": ");
            if (colonPos == CircularReader::npos)
                return false;

            const CircularReaderSubstr &headerName  = hdr.substr(0,colonPos);
            const CircularReaderSubstr &headerValue = hdr.substr(colonPos+2);

            if (headerName.icaseMatch("connection"))
            {
                if (headerValue.icaseFind("upgrade") != CircularReader::npos)
                {
                    got_flags |= GOT_CONNECTION_UPGRADE;
                }
            }
            else if (headerName.icaseMatch("upgrade"))
            {
                if (headerValue.icaseFind("websocket") != CircularReader::npos)
                {
                    got_flags |= GOT_UPGRADE_WEBSOCKET;
                }
            }
            else if (headerName.icaseMatch("sec-websocket-accept"))
            {
                if (headerValue.toString() == secWebsocketKeyResponse)
                {
                    got_flags |= GOT_ACCEPT;
                }
            }
        }
    }
    else
    {
        // mime headers complete
        if (got_flags != GOT_ALL)
        {
            return false;
        }
        else
        {
            state = STATE_CONNECTED;
            if (!finished)
            {
                WaitUtil::Lock   lock(this);
                onConnect();
                bUserConnCallback = true;
            }
        }
    }
    return true;
}

bool
WebSocketClient :: handle_message(void)
{
    uint32_t decoded_length;
    uint32_t pos;

    while (1)
    {
        uint32_t readbuf_len = readbuf.size();
        uint32_t header_len = 2;

        if (VERBOSE)
            cout << "handle_message readbuflen " << readbuf_len << endl;

        if (readbuf_len < header_len)
            // not enough yet.
            return true;

        if ((readbuf[0] & 0x80) == 0)
        {
            cerr << "FIN=0 found, segmentation not supported" << endl;
            return false;
        }

        if ((readbuf[1] & 0x80) != 0)
        {
            cerr << "MASK=1 found, illegal for server->client" << endl;
            return false;
        }

        decoded_length = readbuf[1] & 0x7F;
        pos=2;

        // check if there's enough for a full message
        // given what we know so far.
        if (readbuf_len < (decoded_length+header_len))
        {
            if (VERBOSE)
                cout << "bail out case 1" << endl;
            // not enough yet.
            return true;
        }

        if (decoded_length == 126)
        {
            decoded_length = (readbuf[pos] << 8) + readbuf[pos+1];
            pos += 2;
            header_len += 2;
        }
        else if (decoded_length == 127)
        {
            if (readbuf[pos+0] != 0 || readbuf[pos+1] != 0 ||
                readbuf[pos+2] != 0 || readbuf[pos+3] != 0)
            {
                // we do not support ws packets over 4GB, so
                // assume these four bytes are always zero.
                // dump the conn if not.
                fprintf(stderr, "ws packet over 4GB detected, "
                        "dropping connection\n");
                return false;
            }
            pos += 4; // skip the first 4 bytes of the size
            decoded_length =
                (readbuf[pos+0] << 24) + (readbuf[pos+1] << 16) +
                (readbuf[pos+2] <<  8) +  readbuf[pos+3];
            pos += 4;
            header_len += 8;
        }

        if (readbuf_len < (decoded_length+header_len))
        {
            if (VERBOSE)
                cout << "bail out case 2" << endl;
            // still not enough
            return true;
        }

        WebAppMessageType msgType = WS_TYPE_INVALID;
        switch (readbuf[0] & 0xf)
        {
        case 1:  msgType = WS_TYPE_TEXT;    break;
        case 2:  msgType = WS_TYPE_BINARY;  break;
        case 8:  msgType = WS_TYPE_CLOSE;   break;
        default:
            fprintf(stderr, "unhandled websocket opcode %d received\n",
                    readbuf[0] & 0xf);
            return false;
        }

        if (!finished)
        {
            WaitUtil::Lock   lock(this);
            if (onMessage(
                    WebAppMessage(
                        msgType,
                        readbuf.toString(pos, decoded_length))) == false)
            {
                return false;
            }
        }

        pos += decoded_length;
        readbuf.erase0(pos);
    }
    if (VERBOSE)
        cout << "return case 3" << endl;
    return true;
}

/*virtual*/
bool
WebSocketClient :: handleWriteSelect(int fd)
{
    // not presently used
    return false;
}

/*virtual*/
bool
WebSocketClient :: doPoll(void)
{
    // not presently used
    return false;
}

/*virtual*/
void
WebSocketClient :: done(void)
{
    finished = true;
    if (bUserConnCallback)
    {
        WaitUtil::Lock   lock(this);
        onDisconnect();
    }
    bUserConnCallback = false;
}

bool
WebSocketClient :: sendMessage(const WebAppMessage &m)
{
    if (state != STATE_CONNECTED)
    {
        __WSERR(ERR_NOTCONN);
        return false;
    }

    string msg;

    switch (m.type)
    {
    case WS_TYPE_TEXT:
        msg += (char) 0x81;
        break;
    case WS_TYPE_BINARY:
        msg += (char) 0x82;
        break;
    case WS_TYPE_CLOSE:
        msg += (char) 0x88;
        break;
    default:
        fprintf(stderr, "bogus msg type %d\n", m.type);
        return false;
    }

    int len = m.buf.size();

    if (len < 126)
    {
        msg += (char) ((len & 0x7f) | 0x80); // mask bit
    }
    else if (len < 65536)
    {
        msg += (char) (126 | 0x80); // mask bit
        msg += (char) ((len >> 8) & 0xFF);
        msg += (char) ((len >> 0) & 0xFF);
    }
    else
    {
        msg += (char) (127 | 0x80); // mask bit
        msg += (char) 0;
        msg += (char) 0;
        msg += (char) 0;
        msg += (char) 0;
        msg += (char) ((len >> 24) & 0xFF);
        msg += (char) ((len >> 16) & 0xFF);
        msg += (char) ((len >>  8) & 0xFF);
        msg += (char) ((len >>  0) & 0xFF);
    }

    uint32_t mask_value = random();
    uint8_t mask[4];

    mask[0] = (mask_value >> 24) & 0xFF;
    mask[1] = (mask_value >> 16) & 0xFF;
    mask[2] = (mask_value >>  8) & 0xFF;
    mask[3] = (mask_value >>  0) & 0xFF;

    msg += (char) mask[0];
    msg += (char) mask[1];
    msg += (char) mask[2];
    msg += (char) mask[3];

    size_t  pos = msg.size();
    msg += m.buf;

    for (size_t counter = 0; counter < m.buf.size(); counter++)
        msg[pos + counter] = msg[pos + counter] ^ mask[counter&3];

    if (VERBOSE)
    {
        uint8_t * buf = (uint8_t *) msg.c_str();
        int len = (int) msg.size();
        printf("** write buffer : ");
        for (int ctr = 0; ctr < len; ctr++)
            printf("%02x ", buf[ctr]);
        printf("\n");
    }

    WaitUtil::Lock   lock(this);
    int writeLen = msg.size();
    int cc = ::write(fd, msg.c_str(), writeLen);
    if (cc != writeLen)
    {
        fprintf(stderr, "WebSocketClient :: sendMessage: "
                "write %d returned %d (err %d: %s)\n",
                writeLen, cc, errno, strerror(errno));
    }

    return true;
}

} // namespace WebAppClient
