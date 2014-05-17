/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "WebAppServer.h"
#include "WebAppServerInternal.h"
#include "md5.h"
#include "sha1.h"
#include "base64.h"

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>

#define VERBOSE 0

using namespace std;

namespace WebAppServer {

WebSocketConnection :: WebSocketConnection(
    serverPort::ConfigRecList_t &_configs, int _fd)
    : WebServerConnectionBase(_configs, _fd)
{
    state = STATE_HEADER;
    got_flags = GOT_NONE;
    // i wanted to startFdThread in WebServerConnectionBase, but i can't
    // because it would call pure virtual methods that aren't set up until
    // this derived contructor is complete. so.. bummer.
    startFdThread(_fd);
}

WebSocketConnection :: ~WebSocketConnection(void)
{
}

//virtual
bool
WebSocketConnection :: handleSomeData(void)
{
    if (state == STATE_HEADER)
        if (handle_header() == false)
            return false;
    if (state == STATE_CONNECTED)
        if (handle_message() == false)
            return false;
    return true;
}

//virtual
bool
WebSocketConnection :: doPoll(void)
{
    if (wac)
        return wac->doPoll();
    return true;
}

static const char websocket_guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

bool
WebSocketConnection :: handle_header(void)
{
    while (1)
    {
        size_t newline_pos = readbuf.find("\r\n");
        if (newline_pos == CircularReader::npos)
            // not enough present to find a line.
            break;

        if (newline_pos > 0)
            if (handle_header_line(readbuf.substr(0,newline_pos)) == false)
                return false;

        readbuf.erase0(newline_pos+2);

        if (newline_pos == 0)
        {
            // found empty newline: mime headers are complete?
            if (got_flags == GOT_ALL)
            {
                cout << "found header version 2, checking configs" << endl;

                if (findResource())
                {
                    cout << "sending handshake" << endl;
                    send_handshake_response();
                    // end of the MIME headers means time to respond
                    // to the client.
                    state = STATE_CONNECTED;
                    // don't keep trying to process text headers, 
                    // at this point we're switching to binary encoding.
                    wac = config->cb->newConnection();
                    setPollInterval(config->pollInterval);
                    registerWithWebAppConn(wac);
                    return true;
                }

                cout << "no matching config for route found" << endl;
            }
            else
            {
                cout << "missing required header, got flags : "
                     << hex << got_flags << endl;
            }

            return false;
        }
    }
    return true;
}

bool
WebSocketConnection :: handle_header_line(
    const CircularReaderSubstr &headerLine)
{
    if (VERBOSE)
        cout << "got header: " << headerLine << endl;

    if (headerLine.compare(0,"GET "))
    {
        size_t secondSpacePos = headerLine.find_first_of(' ', 4);
        if (secondSpacePos == CircularReader::npos)
        {
            cerr << "bogus GET line, bailing" << endl;
            return false;
        }
        resource = headerLine.toString(4,secondSpacePos-4);
        cout << "resource is '" << resource << "'" << endl;
        got_flags |= GOT_RESOURCE;
    }
    else
    {
        size_t colonPos = headerLine.find(": ");
        if (colonPos == CircularReader::npos)
        {
            cerr << "no colon: bailing out" << endl;
            return false;
        }

        const CircularReaderSubstr headerName = headerLine.substr(0,colonPos);
        const CircularReaderSubstr headerValue = headerLine.substr(colonPos+2);

        if (VERBOSE)
            cout << "checking header '" << headerName
                 << "' with value '" << headerValue << "'" << endl;

        if (headerName == "Host")
        {
            got_flags |= GOT_HOST;
            host = headerValue.toString();
            if (VERBOSE)
                cout << "HOST is : '" << host << "'" << endl;
        }
        else if (headerName == "Connection")
        {
            // Connection can be a comma separated list.
            if (headerValue.icaseFind("upgrade") != CircularReader::npos)
            {
                got_flags |= GOT_CONNECTION_FLAG;
            }
        }
        else if (headerName == "Upgrade")
        {
            if (headerValue.icaseFind("websocket") != CircularReader::npos)
            {
                got_flags |= GOT_UPGRADE_FLAG;
            }
        }
        else if (headerName == "Origin")
        {
            got_flags |= GOT_ORIGIN;
            origin = headerValue.toString();
            if (VERBOSE)
                cout << "got origin '" << origin << "'" << endl;
        }
        else if (headerName == "Sec-WebSocket-Version")
        {
            got_flags |= GOT_VERSION;
            version = headerValue.toString();
            if (VERBOSE)
                cout << "got version '" << version << "'" << endl;
        }
        else if (headerName == "Sec-WebSocket-Key")
        {
            got_flags |= GOT_KEY;
            key = headerValue.toString();
            if (VERBOSE)
                cout << "got key '" << key << "'" << endl;
        }
        else if (headerName == "Sec-WebSocket-Key1" ||
                 headerName == "Sec-WebSocket-Key2")
        {
            cerr << "ERROR don't support hixie-76" << endl;
            return false;
        }
    }
    return true;
}

void
WebSocketConnection :: send_handshake_response(void)
{
    ostringstream tempbuf;
    tempbuf << key << websocket_guid;

    SHA1Context  ctx;
    uint8_t digest[SHA1HashSize];
    uint8_t digest_b64[128];

    SHA1Reset( &ctx );
    SHA1Input( &ctx,
               (const uint8_t*) tempbuf.str().c_str(),
               tempbuf.str().size() );
    SHA1Result( &ctx, digest );

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

// i could implement sockets on top of ios, a la 
//   http://users.utu.fi/lanurm/socket++/
// but why?

    write(fd, out_frame.c_str(), out_frame.size());
}

//
//   websocket message format:   (see RFC 6455)
//
//   +-7-+-6-+-5-+-4-+-3---2---1---0-+
//   | F | R | R | R |               |  FIN = final of segmented msg
//   | I | S | S | S |    OPCODE     |  OPCODE = 1=text 2=bin 8=close 9=ping
//   | N | V | V | V |               |           a=pong 3-7,b-f=rsvd
//   |   | 1 | 2 | 3 |               |           0=continuation frame
//   +---+---+---+---+---------------+
//   | M |                           |  MASK = 32-bit xor mask is present
//   | A |    PAYLOAD LEN            |  PAYLOAD LEN = 0-125 length
//   | S |                           |     126 = following 2 bytes is length
//   | K |                           |     127 = following 8 bytes is length
//   +---+---------------------------+
//   | extended length, 2 or 8 bytes |
//   +-------------------------------+
//   |    if MASK, 4-byte mask here  |
//   +-------------------------------+
//   |    PAYLOAD                    |
//   +-------------------------------+
//
//  mask is always present client -> server,
//  mask is never present server -> client.
//  we don't support segmentation, so we error out.
//

bool
WebSocketConnection :: handle_message(void)
{
    uint32_t decoded_length;
    uint32_t pos;

    while (1)
    {
        uint32_t readbuf_len = readbuf.size();

        if (readbuf_len < 2)
            // not enough yet.
            return true;

        if (VERBOSE)
        {
            int sz = readbuf.size();
            char printbuf[sz];
            readbuf.copyOut(printbuf,0,sz);
            printf("got msg : ");
            for (uint32_t c = 0; c < sz; c++)
                printf("%02x ", printbuf[c]);
            printf("\n");
        }

        if ((readbuf[0] & 0x80) == 0)
        {
            cerr << "FIN=0 found, segmentation not supported" << endl;
            return false;
        }

        if ((readbuf[1] & 0x80) == 0)
        {
            cerr << "MASK=0 found, illegal for client->server" << endl;
            return false;
        }

        decoded_length = readbuf[1] & 0x7F;
        pos=2;

        // length is size of payload, add 2 for header.
        // note bufsize needs to be rechecked if this is actually an
        // extended-length frame. this logic works for those cases
        // because if we're using 2 or 4 bytes for extended length,
        // then at least 126 needs to be present anyway.
        if (readbuf_len < (decoded_length+2))
            // not enough yet.
            return true;

        if (decoded_length == 126)
        {
            decoded_length = (readbuf[pos] << 8) + readbuf[pos+1];
            pos += 2;
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
        }

        if (readbuf_len < (decoded_length+2))
            // still not enough
            return true;

        uint8_t mask[4];
        uint32_t counter;
        for (counter = 0; counter < 4; counter++)
            mask[counter] = readbuf[pos + counter];
        pos += 4;

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

        for (counter = 0; counter < decoded_length; counter++)
            readbuf[pos + counter] =
                readbuf[pos + counter] ^ mask[counter&3];

        if (wac)
            if (wac->onMessage(
                    WebAppMessage(
                        msgType,
                        readbuf.toString(pos,decoded_length))) == false)
                return false;

        pos += decoded_length;
        readbuf.erase0(pos);
    }

    return true;
}

void
WebSocketConnection :: sendMessage(const WebAppMessage &m)
{
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
        return;
    }

    int len = m.buf.size();

    if (len < 126)
    {
        msg += (char) (len & 0x7f);
    }
    else if (len < 65536)
    {
        msg += (char) 126;
        msg += (char) ((len >> 8) & 0xFF);
        msg += (char) ((len >> 0) & 0xFF);
    }
    else
    {
        msg += (char) 127;
        msg += (char) ((len >> 24) & 0xFF);
        msg += (char) ((len >> 16) & 0xFF);
        msg += (char) ((len >>  8) & 0xFF);
        msg += (char) ((len >>  0) & 0xFF);
    }

    msg += m.buf;

    const uint8_t * buf = (const uint8_t *) msg.c_str();
    // repurpose len
    len = msg.size();

    if (VERBOSE)
    {
        printf("** write buffer : ");
        for (int ctr = 0; ctr < len; ctr++)
            printf("%02x ", buf[ctr]);
        printf("\n");
    }

    write(fd, buf, len);
}

} // namespace WebAppServer
