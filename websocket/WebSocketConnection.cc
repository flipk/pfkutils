
#include "WebSocketServer.h"
#include "sha1.h"
#include "base64.h"
#include "md5.h"

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

WebSocketConnection :: WebSocketConnection(int _fd)
{
    fd = _fd;
    state = STATE_HEADER;
    host = key = key1 = key2 = origin = version = resource = NULL;
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
    if (key1)     delete[] key1;
    if (key2)     delete[] key2;
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
    protoversion = 2; // assume rfc6455 until we know it's hixie-76
    while (!done)
    {
        int cc = read(fd, buf + bufsize, maxbufsize - bufsize);
        if (cc < 0)
            fprintf(stderr, "read : %s\n", strerror(errno));
        if (cc <= 0)
            break;

#if 0
        printf("**read returns %d : ", cc);
        for (int ctr = 0; ctr < cc; ctr++)
            printf("%02x ", buf[ctr]);
        printf("\n");
#endif

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
        if (0)                                                          \
            printf("comparing '%s' to '%s' : %d\n",                     \
                   (char*) line, hdr, ret);                             \
        ret;                                                            \
    })

void
WebSocketConnection :: handle_header(void)
{
    while (1)
    {
        char * newline = strstr((char*)buf, (char*)"\r\n");
        if (newline == NULL)
            // not enough present to find a line.
            break;

        // matchlen does NOT include the \r\n at the end.
        int matchlen = newline - (char*)buf;
        if (protoversion == 1 && matchlen == 0 && bufsize < 10)
        {
            // the key3 is not yet arrived
            break;
        }

        *newline = 0;

#if 0
        printf("got header: %s\n", buf);
#endif

        // handle header line
        if (matchlen == 0)
        {
            bool ready = false;

            if (host && key1 && key2 && origin && 
                resource && upgrade_flag && connection_flag)
            {
                protoversion = 1;
                printf("found header version 1, sending handshake\n");
                send_handshake_response();
                memcpy(key3, buf+2, sizeof(key3));
                printf("found key3: ");
                for (int ctr = 0; ctr < (int)sizeof(key3); ctr++)
                    printf("%02x ", key3[ctr]);
                printf("\n");
                if (bufsize > 10)
                    memmove(buf, newline + 10, bufsize - 10);
                ready = true;
            }

            if (host && key && origin && version &&
                resource && upgrade_flag && connection_flag)
            {
                protoversion = 2;
//                printf("found header version 2, sending handshake\n");
                send_handshake_response();
                if (bufsize > 2)
                    memmove(buf, newline + 2, bufsize - 2);
                ready = true;
            }

            if (ready)
            {
                bufsize -= 2;
                // end of the MIME headers means time to respond
                // to the client.
                state = STATE_CONNECTED;
                // don't keep trying to process text headers, 
                // at this point we're switching to binary encoding.
                onReady();
            }
            else
            {
                fprintf(stderr,"missing required header, bailing out\n");
                done = true;
            }
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
        }
        else if (HEADERMATCH(buf, "Origin: "))
        {
            origin = new char[strlen((char*)buf + 8) + 1];
            strcpy(origin, (char*)buf + 8);
        }
        else if (HEADERMATCH(buf, "Sec-WebSocket-Version: "))
        {
            version = new char[strlen((char*)buf + 23) + 1];
            strcpy(version, (char*)buf + 23);
        }
        else if (HEADERMATCH(buf, "Sec-WebSocket-Key: "))
        {
            key = new char[strlen((char*)buf + 19) + 1];
            strcpy(key, (char*)buf + 19);
        }
        else if (HEADERMATCH(buf, "Sec-WebSocket-Key1: "))
        {
            int len = strlen((char*)buf + 20);
            printf("found key1 of length %d: ", len);
            for (int ctr = 0; ctr < len; ctr++)
                printf("%02x ", buf[ctr+20]);
            printf("\n");
            key1 = new char[len + 1];
            strcpy(key1, (char*)buf + 20);
            protoversion = 1; // we know now it's hixie-76
        }
        else if (HEADERMATCH(buf, "Sec-WebSocket-Key2: "))
        {
            int len = strlen((char*)buf + 20);
            printf("found key2 of length %d: ", len);
            for (int ctr = 0; ctr < len; ctr++)
                printf("%02x ", buf[ctr+20]);
            printf("\n");
            key2 = new char[len + 1];
            strcpy(key2, (char*)buf + 20);
        }

        // now we include the \r\n with all these 2's.
        memmove(buf, newline + 2, bufsize - matchlen - 2);
        bufsize -= matchlen + 2;
    }
}

static uint32_t doStuffToObtainAnInt32(const char *key)
{
    char res_decimals[15] = "";
    char *tail_res = res_decimals;
    uint8_t space_count = 0;
    uint8_t i = 0;
    do {
        if (isdigit(key[i]))
            strncat(tail_res++, &key[i], 1);
        if (key[i] == ' ')
            space_count++;
    } while (key[++i]);

    return ((uint32_t) strtoul(res_decimals, NULL, 10) / space_count);
}

void
WebSocketConnection :: send_handshake_response(void)
{
    char out_frame[1024];
    int written;

    if (protoversion == 1)
    {

        // xxx this obeys the hixie-76 spec but doesn't seem 
        // to work with safari.

        written = snprintf(
            out_frame, sizeof(out_frame),
            "HTTP/1.1 101 WebSocket Protocol Handshake\r\n"
            "Upgrade: WebSocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Origin: %s\r\n"
            "Sec-WebSocket-Location: ws://%s%s\r\n"
            "\r\n"
            , origin, host, resource);

        printf("returned headers: %s", out_frame);

	uint32_t ikey1 = doStuffToObtainAnInt32(key1);
	uint32_t ikey2 = doStuffToObtainAnInt32(key2);

        printf("ikey1 = %u,  ikey2 = %u\n", ikey1, ikey2);

	uint8_t chrkey1[4];
	uint8_t chrkey2[4];
	uint8_t i;

	for (i = 0; i < 4; i++)
		chrkey1[i] = ikey1 << (8 * i) >> (8 * 3);
	for (i = 0; i < 4; i++)
		chrkey2[i] = ikey2 << (8 * i) >> (8 * 3);

	uint8_t raw_md5[16];
	uint8_t keys[16];

	memcpy(keys, chrkey1, 4);
	memcpy(&keys[4], chrkey2, 4);
	memcpy(&keys[8], key3, 8);
	md5(raw_md5, keys, sizeof (keys)*8);

        write(fd, out_frame, written);
        write(fd, raw_md5, sizeof(keys));
    }

    if (protoversion == 2)
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

        written = sprintf(
            out_frame,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n\r\n",
            digest_b64);

        write(fd, out_frame, written);
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
    uint32_t decoded_length;
    uint32_t pos;

    if (protoversion == 1)
    {
        while (1)
        {
            if (bufsize == 0)
                break;
            if (buf[0] == 0x00)
            {
                for (pos = 0; pos < bufsize; pos++)
                    if (buf[pos] == 0xFF)
                        break;
                if (pos == bufsize)
                    // not enough yet
                    return;
                pos++;
                m.type = WS_TYPE_TEXT;
                m.buf = buf + 1;
                m.len = pos - 2;
                onMessage(m);
            }
            else if (buf[0] == 0x80)
            {
                // xxx handle

                if (bufsize < 3)
                    // not enough
                    return;

                if (buf[1] & 0x80)
                {
                    decoded_length = ((buf[1] & 0x7f) << 7) + (buf[2] & 0x7f);
                    pos = 3;
                }
                else
                {
                    decoded_length = buf[1];
                    pos = 2;
                }
                m.type = WS_TYPE_BINARY;
                m.buf = buf + pos;
                m.len = decoded_length;
                pos += decoded_length;
                onMessage(m);
            }
            else
            {
                printf("bogus byte %02x at input\n", buf[0]);
                done = true;
                return;
            }
            if (bufsize > pos)
                memmove(buf, buf + pos, bufsize - pos);
            bufsize -= pos;
        }
    }
    else if (protoversion == 2)
    {
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

            decoded_length = buf[1] & 0x7F;
            pos=2;

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
            pos += decoded_length;

            uint32_t counter;
            for (counter = 0; counter < decoded_length; counter++)
                m.buf[counter] ^= mask[counter & 3];

            onMessage(m);

            if (bufsize > pos)
                memmove(buf, buf + pos, bufsize - pos);
            bufsize -= pos;
        }
    }
}

void
WebSocketConnection :: sendMessage(const WebSocketMessage &m)
{
    if (protoversion == 1)
    {
        if (m.type == WS_TYPE_TEXT)
        {
            uint8_t c = 0;
            write(fd, (char *) &c, 1);
            write(fd, (char *)m.buf, (int) m.len);
            c = 0xFF;
            write(fd, (char *) &c, 1);

            printf("**write bytes: 00 ");
            for (int ctr = 0; ctr < m.len; ctr++)
                printf("%02x ", m.buf[ctr]);
            printf("FF\n");
        }
        else if (m.type == WS_TYPE_BINARY)
        {
            uint8_t c = 0x80;
            write(fd, (char *) &c, 1);

            printf("**write bytes: 80 ");
            if (m.len > 127)
            {
                c = 0x80 | (m.len / 128);
                printf("%02x ", c);
                write(fd, (char *) &c, 1);
            }
            c = m.len & 0x7f;
            printf("%02x ", c);
            write(fd, (char *) &c, 1);

            write(fd, (char *)m.buf, (int) m.len);

            for (int ctr = 0; ctr < m.len; ctr++)
                printf("%02x ", m.buf[ctr]);
            printf("\n");

            // binary has no training 0xFF
        }
    }
    else if (protoversion == 2)
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

#if 0
        printf("** write buffer (%d): ", hdrlen + m.len);
        for (int ctr = 0; ctr < hdrlen; ctr++)
            printf("%02x ", hdr[ctr]);
        for (int ctr = 0; ctr < m.len; ctr++)
            printf("%02x ", m.buf[ctr]);
        printf("\n");
#endif
        write(fd, hdr, hdrlen);
        write(fd, (char*)m.buf, m.len);
    }
}
