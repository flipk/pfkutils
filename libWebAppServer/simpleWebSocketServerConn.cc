
#include "simpleWebSocket.h"
#include "base64.h"

#include <sstream>
#include <mbedtls/sha1.h>
#include <errno.h>

#define VERBOSE 1

using namespace std;

namespace SimpleWebSocket {

WebSocketServerConn :: WebSocketServerConn(int _fd,
                                         const struct sockaddr_in &_sa)
    : WebSocketConn(_fd, true)
{
    state = STATE_HEADER;
    got_flags = 0;
}

/*virtual*/
WebSocketServerConn :: ~WebSocketServerConn(void)
{
}

/*virtual*/ WebSocketRet
WebSocketServerConn :: handle_data(::google::protobuf::Message &msg)
{
    switch (state)
    {
    case STATE_HEADER:
        return handle_header();
    case STATE_CONNECTED:
        return handle_message(msg);
    default:
        cerr << "WebSocketServerConn wut\n";
    }

    return WEBSOCKET_NO_MESSAGE;
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
    if (VERBOSE)
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

        if (VERBOSE)
            cout << "checking header '" << headerName
                 << "' with value '" << headerValue << "'" << endl;

        if (headerName.icaseMatch("host"))
        {
            got_flags |= GOT_HOST;
            host = headerValue.toString();
            if (VERBOSE)
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
            if (VERBOSE)
                cout << "got origin '" << origin << "'" << endl;
        }
        else if (headerName.icaseMatch("sec-websocket-version"))
        {
            got_flags |= GOT_VERSION;
            version = headerValue.toString();
            if (VERBOSE)
                cout << "got version '" << version << "'" << endl;
        }
        else if (headerName.icaseMatch("sec-websocket-key"))
        {
            got_flags |= GOT_KEY;
            key = headerValue.toString();
            if (VERBOSE)
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

    if (VERBOSE)
        cout << "writing headers: " << out_frame << endl;

    if (::write(fd, out_frame.c_str(), out_frame.size()) < 0)
    {
        fprintf(stderr, "send_handshake_response: write failed: %d (%s)\n",
                errno, strerror(errno));
    }
}

WebSocketRet
WebSocketServerConn :: handle_message(::google::protobuf::Message &msg)
{
    uint32_t decoded_length;
    uint32_t pos;

    while (1)
    {
        uint32_t readbuf_len = readbuf.size();
        uint32_t header_len = 2;

        if (readbuf_len < header_len)
            // not enough yet.
            return WEBSOCKET_NO_MESSAGE;

        if (VERBOSE)
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

        if ((readbuf[1] & 0x80) == 0)
        {
            cerr << "MASK=0 found, illegal for client->server" << endl;
            return WEBSOCKET_CLOSED;
        }
        header_len += 4; // account for mask

        decoded_length = readbuf[1] & 0x7F;
        pos=2;

        // check if there's enough for a full message
        // given what we know so far.
        if (readbuf_len < (decoded_length+header_len))
            // not enough yet.
            return WEBSOCKET_NO_MESSAGE;

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
            // still not enough
            return WEBSOCKET_NO_MESSAGE;

        uint8_t mask[4];
        uint32_t counter;
        for (counter = 0; counter < 4; counter++)
            mask[counter] = readbuf[pos + counter];
        pos += 4;

        int opcode = (readbuf[0] & 0xf);
        if (opcode != 2) // WS_TYPE_BINARY
        {
            if (opcode != 8) // WS_TYPE_CLOSE
                fprintf(stderr, "unhandled websocket opcode %d received\n",
                        opcode);
            return WEBSOCKET_CLOSED;
        }

        for (counter = 0; counter < decoded_length; counter++)
            readbuf[pos + counter] =
                readbuf[pos + counter] ^ mask[counter&3];

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

    return WEBSOCKET_NO_MESSAGE;
}

};
