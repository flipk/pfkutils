
#include "WebSocketServer.h"
#include "sha1.h"
#include "base64.h"

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

WebSocketConnection :: WebSocketConnection(int _fd)
{
    fd = _fd;
    state = STATE_HEADER;
    host = key = origin = version = resource = NULL;
    upgrade_flag = connection_flag = false;
    done = false;
    bufsize = 0;
}

//virtual
WebSocketConnection :: ~WebSocketConnection(void)
{
    close(fd);
    if (host)     delete[] host;
    if (key)      delete[] key;
    if (origin)   delete[] origin;
    if (version)  delete[] version;
    if (resource) delete[] resource;
}

void
WebSocketConnection :: connection_thread_main(void)
{
    bufsize = 0;
    state = STATE_HEADER;
    host = key = origin = version = resource = NULL;
    upgrade_flag = connection_flag = false;
    done = false;
    while (!done)
    {
        printf("calling read on fd %d\n", fd);
        int cc = read(fd, buf + bufsize, maxbufsize - bufsize);
        printf("read on fd %d returned %d\n", fd, cc);
        if (cc < 0)
            fprintf(stderr, "read : %s\n", strerror(errno));
        if (cc <= 0)
            break;
        bufsize += cc;
        buf[bufsize] = 0; // safe because of +1 in defn of buf
        switch (state)
        {
        case STATE_HEADER:  handle_header(); break;
        case STATE_CONNECTED: handle_message(); break;
        }
    }
}

static const char websocket_guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

#define HEADERMATCH(line,hdr)                                           \
    ({                                                                  \
        int ret = (strncasecmp((char*)line, hdr, sizeof(hdr)-1) == 0);  \
        printf("comparing '%s' to '%s' : %d\n",                         \
               (char*) line, hdr, ret);                                 \
        ret;                                                            \
    })

void
WebSocketConnection :: handle_header(void)
{
    while (1)
    {
        printf("handle_header : starting with bufsize %d\n", bufsize);

        char * newline = strstr((char*)buf, (char*)"\r\n");
        if (newline == NULL)
            // not enough present to find a line.
            break;

        // matchlen does NOT include the \r\n at the end.
        int matchlen = newline - (char*)buf;
        *newline = 0;

        printf("found matchlen %d header: '%s'\n", matchlen, buf);

        // handle header line
        if (matchlen == 0)
        {
            printf("found end of MIME\n");
            if (host == NULL || key == NULL ||
                origin == NULL || version == NULL ||
                resource == NULL || upgrade_flag == false ||
                connection_flag == false)
            {
                printf("missing required header, bailing out\n");
                done = true;
                return;
            }
            printf("successful parsing of handshake header\n");
            // end of the MIME headers means time to respond
            // to the client.
            send_handshake_response();
            state = STATE_CONNECTED;
            // don't keep trying to process text headers, 
            // at this point we're switching to binary encoding.
            return;
        }
        else if (HEADERMATCH(buf, "GET "))
        {
            char * space = strstr((char*)buf + 4, " ");
            if (space)
            {
                *space = 0;
                resource = new char[strlen((char*)buf + 4) + 1];
                strcpy(resource, (char*) buf + 4);
                printf("resource is '%s'\n", resource);
            }
        }
        else if (HEADERMATCH(buf, "Connection: upgrade") ||
                 HEADERMATCH(buf, "Connection: keep-alive, Upgrade"))
        {
            connection_flag = true;
        }
        else if (HEADERMATCH(buf, "Upgrade: websocket"))
        {
            upgrade_flag = true;
        }
        else if (HEADERMATCH(buf, "Host: "))
        {
            host = new char[strlen((char*)buf + 6) + 1];
            strcpy(host, (char*)buf+6);
            printf("host is '%s'\n", host);
        }
        else if (HEADERMATCH(buf, "Origin: "))
        {
            origin = new char[strlen((char*)buf + 8) + 1];
            strcpy(origin, (char*)buf + 8);
            printf("origin is '%s'\n", origin);
        }
        else if (HEADERMATCH(buf, "Sec-WebSocket-Version: "))
        {
            version = new char[strlen((char*)buf + 23) + 1];
            strcpy(version, (char*)buf + 23);
            printf("version is '%s'\n", version);
        }
        else if (HEADERMATCH(buf, "Sec-WebSocket-Key: "))
        {
            key = new char[strlen((char*)buf + 19) + 1];
            strcpy(key, (char*)buf + 19);
            printf("key is '%s'\n", key);
        }

        // now we include the \r\n with all these 2's.
        memmove(buf, newline + 2, bufsize - matchlen - 2);
        bufsize -= matchlen + 2;

        printf("bufsize is now %d\n", bufsize);
    }
}

void
WebSocketConnection :: send_handshake_response(void)
{
    char tempbuf[100];
    int len = sprintf(tempbuf, "%s%s", key, websocket_guid);

    SHA1Context  ctx;
    uint8_t digest[SHA1HashSize];
    uint8_t digest_b64[128];

    SHA1Reset( &ctx );
    SHA1Input( &ctx, (uint8_t*)tempbuf, len );
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

    char out_frame[1024];

    unsigned int written = sprintf(
        out_frame,
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n",
        digest_b64);

    write(fd, out_frame, written);    
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
//   | K |                           |     127 = following 4 bytes is length
//   +---+---------------------------+
//   | extended length, 2 or 4 bytes |
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

void
WebSocketConnection :: handle_message(void)
{
    WebSocketMessage m;

    while (1)
    {
        if (bufsize < 2)
            // not enough yet.
            return;

        if ((buf[0] & 0x80) == 0)
        {
            fprintf(stderr, "FIN=0 found, segmentation not supported\n");
            done = true;
            return;
        }

        if ((buf[1] & 0x80) == 0)
        {
            fprintf(stderr, "MASK=0 found, illegal for client->server\n");
            done = true;
            return;
        }

        uint32_t decoded_length = buf[1] & 0x7F;
        int pos=2;

        // length is size of payload, add 2 for header.
        // note bufsize needs to be rechecked if this is actually an
        // extended-length frame. this logic works for those cases
        // because if we're using 2 or 4 bytes for extended length,
        // then at least 126 needs to be present anyway.
        if (bufsize < (decoded_length+2))
            // not enough yet.
            return;

        if (decoded_length == 126)
        {
            decoded_length = (buf[pos] << 8) + buf[pos+1];
            pos += 2;
        }
        else if (decoded_length == 127)
        {
            decoded_length =
                (buf[pos+0] << 24) + (buf[pos+1] << 16) +
                (buf[pos+2] <<  8) +  buf[pos+3];
            pos += 4;
        }

        if (bufsize < (decoded_length+2))
            // still not enough
            return;

        uint8_t * mask = buf + pos;
        pos += 4;

        switch (buf[0] & 0xf)
        {
        case 1:  m.type = WS_TYPE_TEXT;    break;
        case 2:  m.type = WS_TYPE_BINARY;  break;
        case 8:  m.type = WS_TYPE_CLOSE;   break;
        default:
            fprintf(stderr, "unhandled websocket opcode %d received\n",
                    buf[0] & 0xf);
            done = true;
            return;
        }

        m.buf = buf + pos;
        m.len = decoded_length;

        int counter;
        for (counter = 0; counter < decoded_length; counter++)
            m.buf[counter] ^= mask[counter & 3];

        onMessage(m);

        if (bufsize > pos)
            memmove(buf, buf + pos, bufsize - pos);
        bufsize -= pos;
    }
}

void
WebSocketConnection :: sendMessage(const WebSocketMessage &m)
{
    uint8_t hdr[6]; // opcode, payload len max size (no mask)
    int hdrlen = 0;

    switch (m.type)
    {
    case WS_TYPE_TEXT:
        hdr[hdrlen++] = 0x81;
        break;
    case WS_TYPE_BINARY:
        hdr[hdrlen++] = 0x82;
        break;
    case WS_TYPE_CLOSE:
        hdr[hdrlen++] = 0x88;
        break;
    default:
        fprintf(stderr, "bogus msg type %d\n", m.type);
        return;
    }

    if (m.len < 126)
    {
        hdr[hdrlen++] = m.len & 0x7f;
    }
    else if (m.len < 65536)
    {
        hdr[hdrlen++] = 126;
        hdr[hdrlen++] = (m.len >> 8) & 0xFF;
        hdr[hdrlen++] = (m.len >> 0) & 0xFF;
    }
    else
    {
        hdr[hdrlen++] = 127;
        hdr[hdrlen++] = (m.len >> 24) & 0xFF;
        hdr[hdrlen++] = (m.len >> 16) & 0xFF;
        hdr[hdrlen++] = (m.len >>  8) & 0xFF;
        hdr[hdrlen++] = (m.len >>  0) & 0xFF;
    }

    write(fd, hdr, hdrlen);
    write(fd, (char*)m.buf, m.len);
}
