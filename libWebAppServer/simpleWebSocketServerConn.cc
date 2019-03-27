
#include "simpleWebSocket.h"
#include "base64.h"

#include <sstream>
#include <mbedtls/sha1.h>
#include <errno.h>

using namespace std;

namespace SimpleWebSocket {

////////    WebSocketServer   ////////

WebSocketServer :: WebSocketServer( uint16_t port, uint32_t addr,
                                    bool _verbose )
    : _ok(false), fd(-1), verbose(_verbose)
{
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return;
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(addr);
    int v = 1;
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (void*) &v, sizeof( v ));
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        return;
    listen(fd,1);
    _ok = true;
}

WebSocketServer :: ~WebSocketServer(void)
{
    if (fd > 0)
        close(fd);
}

WebSocketServerConn *
WebSocketServer::handle_accept(void)
{
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    int new_fd = accept(fd, (struct sockaddr *)&sa, &salen);
    if (new_fd < 0)
        return NULL;
    return new WebSocketServerConn(new_fd, sa, verbose);
}

////////    WebSocketServerConn   ////////

WebSocketServerConn :: WebSocketServerConn(int _fd,
                                           const struct sockaddr_in &_sa,
                                           bool _verbose)
    : WebSocketConn(_fd, true, _verbose)
{
    state = STATE_HEADER;
    got_flags = 0;
}

/*virtual*/
WebSocketServerConn :: ~WebSocketServerConn(void)
{
}

WebSocketRet
WebSocketServerConn :: handle_header(void)
{
    while (1)
    {
        int newline_pos = readbuf.find("\r\n");
        if (newline_pos == CircularReader::npos)
            // not enough present to find a line.
            break;

        if (newline_pos > 0)
            if (handle_header_line(readbuf.substr(0,newline_pos)) == false)
                return WEBSOCKET_CLOSED;

        readbuf.erase0(newline_pos+2);

        if (newline_pos == 0)
        {
            // found empty newline: mime headers are complete?
            if (got_flags == GOT_ALL)
            {
                cout << "sending handshake" << endl;
                send_handshake_response();
                // end of the MIME headers means time to respond
                // to the client.
                state = STATE_CONNECTED;
                // don't keep trying to process text headers,
                // at this point we're switching to binary encoding.
                _ok = true;
                return WEBSOCKET_CONNECTED;
            }
            else
            {
                cout << "missing required header, got flags : "
                     << hex << got_flags << endl;
            }

            return WEBSOCKET_CLOSED;
        }
    }
    return WEBSOCKET_NO_MESSAGE;
}

bool
WebSocketServerConn :: handle_header_line(
    const CircularReaderSubstr &headerLine)
{
    if (verbose)
        cout << "got header: " << headerLine << endl;

    if (headerLine.compare(0,"GET "))
    {
        int secondSpacePos = headerLine.find_first_of(' ', 4);
        if (secondSpacePos == CircularReader::npos)
        {
            cerr << "bogus GET line, bailing" << endl;
            return false;
        }
        path = headerLine.toString(4,secondSpacePos-4);
        cout << "resource is '" << path << "'" << endl;
        got_flags |= GOT_RESOURCE;
    }
    else
    {
        int colonPos = headerLine.find(": ");
        if (colonPos == CircularReader::npos)
        {
            cerr << "no colon: bailing out" << endl;
            return false;
        }

        const CircularReaderSubstr &headerName =
            headerLine.substr(0,colonPos);
        const CircularReaderSubstr &headerValue =
            headerLine.substr(colonPos+2);

        if (verbose)
            cout << "checking header '" << headerName
                 << "' with value '" << headerValue << "'" << endl;

        if (headerName.icaseMatch("host"))
        {
            got_flags |= GOT_HOST;
            host = headerValue.toString();
            if (verbose)
                cout << "HOST is : '" << host << "'" << endl;
        }
        else if (headerName.icaseMatch("connection"))
        {
            // Connection can be a comma separated list.
            if (headerValue.icaseFind("upgrade") != CircularReader::npos)
            {
                got_flags |= GOT_CONNECTION_FLAG;
            }
        }
        else if (headerName.icaseMatch("upgrade"))
        {
            if (headerValue.icaseFind("websocket") != CircularReader::npos)
            {
                got_flags |= GOT_UPGRADE_FLAG;
            }
        }
        else if (headerName.icaseMatch("origin"))
        {
// refer to:
//  https://bugzilla.mozilla.org/show_bug.cgi?id=1301156
//  https://bugzilla.mozilla.org/show_bug.cgi?id=1277496
// firefox sometimes sends "Origin" in lower case. inorite?
            got_flags |= GOT_ORIGIN;
            origin = headerValue.toString();
            if (verbose)
                cout << "got origin '" << origin << "'" << endl;
        }
        else if (headerName.icaseMatch("sec-websocket-version"))
        {
            got_flags |= GOT_VERSION;
            version = headerValue.toString();
            if (verbose)
                cout << "got version '" << version << "'" << endl;
        }
        else if (headerName.icaseMatch("sec-websocket-key"))
        {
            got_flags |= GOT_KEY;
            key = headerValue.toString();
            if (verbose)
                cout << "got key '" << key << "'" << endl;
        }
        else if (headerName.icaseMatch("sec-websocket-key1") ||
                 headerName.icaseMatch("sec-websocket-key2"))
        {
            cerr << "ERROR don't support hixie-76" << endl;
            return false;
        }
    }
    return true;
}

void
WebSocketServerConn :: send_handshake_response(void)
{
    ostringstream tempbuf;
    tempbuf << key << websocket_guid;

#define SHA1HashSize 20
    uint8_t digest[SHA1HashSize];
    uint8_t digest_b64[128];

    mbedtls_sha1( (const unsigned char *) tempbuf.str().c_str(),
                  tempbuf.str().size(), digest );

    memset(digest_b64,  0, sizeof(digest_b64));
    int i, o;
    for (i = 0, o = 0; i < SHA1HashSize; i += 3, o += 4)
    {
        int len = SHA1HashSize - i;
        if (len > 3)
            len = 3;
        b64_encode_quantum(digest + i, len, digest_b64 + o);
    }

    string out_frame = (string)
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + ((char*) digest_b64) + "\r\n" +
        "\r\n";

    if (verbose)
        cout << "writing headers: " << out_frame << endl;

    if (::write(fd, out_frame.c_str(), out_frame.size()) < 0)
    {
        fprintf(stderr, "send_handshake_response: write failed: %d (%s)\n",
                errno, strerror(errno));
    }
}

};
