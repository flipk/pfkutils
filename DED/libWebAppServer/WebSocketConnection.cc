/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
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
#include "WebAppServer.h"
#include "WebAppServerInternal.h"
#include "mbedtls/sha1.h"
#include "base64.h"
//#include "md5.h"  // if i ever fix hixie-76

#include <sstream>
#include <errno.h>

#define VERBOSE 0

using namespace std;

namespace WebAppServer {

WebSocketConnection :: WebSocketConnection(
    serverPort::ConfigRecList_t &_configs, int _fd,
    const struct sockaddr_in *sa)
    : WebServerConnectionBase(_configs, _fd)
{
    state = STATE_HEADER;
    got_flags = GOT_NONE;
    memcpy(&remote_addr, sa, sizeof(struct sockaddr_in));
    reassembly_msg_type = WS_TYPE_INVALID;
}

WebSocketConnection :: ~WebSocketConnection(void)
{
    if (wac != NULL)
    {
        wac->onDisconnect();
        delete wac;
    }
    wac = NULL;
}

//virtual
void
WebSocketConnection :: startServer(void)
{
    // i wanted to startFdThread in WebServerConnectionBase, but i can't
    // because it would call pure virtual methods that aren't set up until
    // this derived contructor is complete. so.. bummer.
    startFdThread(tempFd, 1000);    
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

//virtual
void
WebSocketConnection :: done(void)
{
    if (wac)
    {
        wac->onDisconnect();
        delete wac;
    }
    wac = NULL;
    deleteMe = true;
}

const std::string websocket_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

bool
WebSocketConnection :: handle_header(void)
{
    while (1)
    {
        int newline_pos = readbuf.find("\r\n");
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
                    wac->connData = new WebAppConnectionDataWebsocket(this);
                    setPollInterval(config->pollInterval);
                    wac->onConnect();
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
        int secondSpacePos = headerLine.find_first_of(' ', 4);
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
        int colonPos = headerLine.find(": ");
        if (colonPos == CircularReader::npos)
        {
            cerr << "no colon: bailing out" << endl;
            return false;
        }

        const CircularReaderSubstr &headerName = headerLine.substr(0,colonPos);
        const CircularReaderSubstr &headerValue = headerLine.substr(colonPos+2);

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
WebSocketConnection :: send_handshake_response(void)
{
    ostringstream tempbuf;
    tempbuf << key << websocket_guid;

#define SHA1HashSize 20
    uint8_t digest[SHA1HashSize];
    char digest_b64[128];

    MBEDTLS_SHA1( (const unsigned char *) tempbuf.str().c_str(),
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
        "Sec-WebSocket-Accept: " + digest_b64 + "\r\n" +
        "\r\n";

    if (VERBOSE)
        cout << "writing headers: " << out_frame << endl;

    if (::write(fd, out_frame.c_str(), out_frame.size()) < 0)
    {
        fprintf(stderr, "send_handshake_response: write failed: %d (%s)\n",
                errno, strerror(errno));
    }
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
        uint32_t header_len = 2;
        bool final_segment = true;

        if (readbuf_len < header_len)
            // not enough yet.
            return true;

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
            final_segment = false;
        }

        if ((readbuf[1] & 0x80) == 0)
        {
            cerr << "MASK=0 found, illegal for client->server" << endl;
            return false;
        }
        header_len += 4; // account for mask

        decoded_length = readbuf[1] & 0x7F;
        pos=2;

        // check if there's enough for a full message
        // given what we know so far.
        if (readbuf_len < (decoded_length+header_len))
            // not enough yet.
            return true;

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
            // still not enough
            return true;

        uint8_t mask[4];
        uint32_t counter;
        for (counter = 0; counter < 4; counter++)
            mask[counter] = readbuf[pos + counter];
        pos += 4;

        switch (readbuf[0] & 0xf)
        {
        case 0:  /* continuation */                     break;
        case 1:  reassembly_msg_type = WS_TYPE_TEXT;    break;
        case 2:  reassembly_msg_type = WS_TYPE_BINARY;  break;
        case 8:  reassembly_msg_type = WS_TYPE_CLOSE;   break;
        default:
            fprintf(stderr, "unhandled websocket opcode %d received\n",
                    readbuf[0] & 0xf);
            return false;
        }

        for (counter = 0; counter < decoded_length; counter++)
            readbuf[pos + counter] =
                readbuf[pos + counter] ^ mask[counter&3];

        reassembly_buffer.append(readbuf.toString(pos,decoded_length));
        pos += decoded_length;
        readbuf.erase0(pos);

        if (final_segment)
        {
            if (wac)
                if (wac->onMessage(
                        WebAppMessage(
                            reassembly_msg_type,
                            reassembly_buffer)) == false)
                    return false;

            // prepare for next segment.
            reassembly_buffer.resize(0);
            reassembly_msg_type = WS_TYPE_INVALID;
        }
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
        msg += (char) 0;
        msg += (char) 0;
        msg += (char) 0;
        msg += (char) 0;
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

    if (::write(fd, buf, len) != len)
    {
        fprintf(stderr, "sendMessage: write failed: %d (%s)\n",
                errno, strerror(errno));
    }
}

void
WebAppConnectionDataWebsocket :: sendMessage(const WebAppMessage &m)
{
    connBase->sendMessage(m);
}

} // namespace WebAppServer
