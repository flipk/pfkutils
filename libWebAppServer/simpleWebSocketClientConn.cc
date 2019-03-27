
#include "simpleWebSocket.h"

#include "base64.h"

#include <mbedtls/sha1.h>
#include <iostream>
#include <sstream>
#include <errno.h>

using namespace std;

namespace SimpleWebSocket {

WebSocketClientConn :: WebSocketClientConn(uint32_t addr, uint16_t port,
                                           const std::string &path,
                                           bool _verbose)
    : WebSocketConn(-1, false, _verbose)
{
    urlPort = port;
    urlIp = addr;
    urlPath = path;
    init_common();
}

WebSocketClientConn :: WebSocketClientConn(const std::string &_url,
                                           bool _verbose)
    : WebSocketConn(-1, false, _verbose)
{
    Url  url(_url);
    if (url.ok() == false)
    {
        cerr << "WebSocketClientConn: url parse error\n";
        return;
    }
    urlPort = url.port;
    urlIp = url.addr;
    urlPath = url.path;
    urlHost = url.hostname;
    init_common();
}

/*virtual*/
WebSocketClientConn :: ~WebSocketClientConn(void)
{
}

void
WebSocketClientConn :: init_common(void)
{
    struct sockaddr_in sa;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(urlPort);
    sa.sin_addr.s_addr = htonl(urlIp);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        fprintf(stderr, "socket failed: %s\n", strerror(errno));
        return;
    }

    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        fprintf(stderr, "connect failed: %s\n", strerror(errno));
        return;
    }

    ostringstream hdrs;

    generateWsHeaders(hdrs);
    state = STATE_HEADER;

    const string &hdrstr = hdrs.str();
    int writeLen = hdrstr.length();
    int cc = ::write(fd, hdrstr.c_str(), writeLen);
    if (cc != writeLen)
    {
        fprintf(stderr, "WebSocketClient :: init_common: "
                "write %d returned %d (err %d: %s)\n",
                writeLen, cc, errno, strerror(errno));
    }

    got_flags = GOT_NONE;
    _ok = true;
}

static const char *wskeychars =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

const std::string
WebSocketClientConn::hostForConn(void) const
{
    if (urlHost.length() > 0)
        return urlHost;
    //else
    char buf[64];
    snprintf(buf,64,"%d.%d.%d.%d",
             (urlIp >> 24) & 0xFF, (urlIp >> 16) & 0xFF,
             (urlIp >>  8) & 0xFF, (urlIp >>  0) & 0xFF);
    return std::string(buf);
}

void
WebSocketClientConn :: generateWsHeaders(ostringstream &hdrs)
{
    secWebsocketKey.clear();

    int ctr;
    for (ctr = 0; ctr < 22; ctr++)
    {
        int c = random() % 52;
        secWebsocketKey += wskeychars[c];
    }
    secWebsocketKey += "==";

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

    if (verbose)
        cout << hdrs.str();

    // calc secWebsocketKeyResponse

    secWebsocketKey += websocket_guid;

#define SHA1HashSize 20
    uint8_t digest[SHA1HashSize];

    mbedtls_sha1( (const unsigned char *) secWebsocketKey.c_str(),
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

WebSocketRet
WebSocketClientConn :: handle_data(::google::protobuf::Message &msg)
{
    while (state == STATE_HEADER)
    {
        int newline_pos = readbuf.find("\r\n");
        if (newline_pos == CircularReader::npos)
            // not enough present to find a line.
            break;

        const CircularReaderSubstr &hdr = readbuf.substr(0,newline_pos);

        if (verbose)
            cout << "got : " << hdr << endl;

        WebSocketRet r = handle_wsheader(hdr);
        readbuf.erase0(newline_pos+2);
        if (r == WEBSOCKET_CLOSED || r == WEBSOCKET_CONNECTED)
            return r;
    }
    if (state == STATE_CONNECTED)
        return handle_message(msg);
    return WEBSOCKET_NO_MESSAGE;
}

WebSocketRet
WebSocketClientConn :: handle_wsheader(const CircularReaderSubstr &hdr)
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
                return WEBSOCKET_CLOSED;
            int nspacepos = hdr.find(" ",spacepos+1);
            if (nspacepos == CircularReader::npos)
                return WEBSOCKET_CLOSED;
            int val = atoi(hdr.toString(spacepos+1,
                                        nspacepos-spacepos-1).c_str());
            if (val != 101)
                return WEBSOCKET_CLOSED;
            got_flags |= GOT_SWITCHING;
        }
        else
        {
            int colonPos = hdr.find(": ");
            if (colonPos == CircularReader::npos)
                return WEBSOCKET_CLOSED;

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
            return WEBSOCKET_CLOSED;
        }
        else
        {
            state = STATE_CONNECTED;
            return WEBSOCKET_CONNECTED;

        }
    }
    return WEBSOCKET_NO_MESSAGE;
}

WebSocketRet
WebSocketClientConn :: handle_message(::google::protobuf::Message &msg)
{
    uint32_t decoded_length;
    uint32_t pos;

    while (1)
    {
        uint32_t readbuf_len = readbuf.size();
        uint32_t header_len = 2;

        if (verbose)
            cout << "handle_message readbuflen " << readbuf_len << endl;

        if (readbuf_len < header_len)
            // not enough yet.
            return WEBSOCKET_NO_MESSAGE;

        if (verbose)
        {
            int sz = readbuf.size();
            uint8_t printbuf[sz];
            readbuf.copyOut(printbuf,0,sz);
            printf("got msg : ");
            for (uint32_t c = 0; (int)c < sz; c++)
                printf("%02x ", printbuf[c]);
            printf("\n");
        }

        if ((readbuf[0] & 0x80) == 0)
        {
            cerr << "FIN=0 found, segmentation not supported" << endl;
            return WEBSOCKET_CLOSED;
        }

        if ((readbuf[1] & 0x80) != 0)
        {
            cerr << "MASK=1 found, illegal for server->client" << endl;
            return WEBSOCKET_CLOSED;
        }

        decoded_length = readbuf[1] & 0x7F;
        pos=2;

        // check if there's enough for a full message
        // given what we know so far.
        if (readbuf_len < (decoded_length+header_len))
        {
            if (verbose)
                cout << "bail out case 1" << endl;
            // not enough yet.
            return WEBSOCKET_NO_MESSAGE;
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
                return WEBSOCKET_CLOSED;
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
            if (verbose)
                cout << "bail out case 2" << endl;
            // still not enough
            return WEBSOCKET_NO_MESSAGE;
        }

        if ((readbuf[0] & 0xf) != 2) // WS_TYPE_BINARY
        {
            fprintf(stderr, "unhandled websocket opcode %d received\n",
                    readbuf[0] & 0xf);
            return WEBSOCKET_CLOSED;
        }

        msg.Clear();
        bool parseOk =
            msg.ParseFromString(readbuf.toString(pos,decoded_length));
        pos += decoded_length;
        readbuf.erase0(pos);
        if (parseOk == false)
            return WEBSOCKET_CLOSED;
        //else
        return WEBSOCKET_MESSAGE;
    }
    if (verbose)
        cout << "return case 3" << endl;
    return WEBSOCKET_NO_MESSAGE;
}

};
