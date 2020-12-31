
#include "simpleWebSocket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <regex.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <ostream>
#include <iostream>
#include <iomanip>

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

using namespace std;

namespace SimpleWebSocket {

const std::string websocket_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

////////   WebSocketConn  ////////

WebSocketConn :: WebSocketConn(int _fd, bool _server, bool _verbose)
    : _ok(false), fd(_fd), server(_server),
      verbose(_verbose), readbuf(MAX_READBUF)
{
    got_flags = 0;
}

WebSocketConn :: ~WebSocketConn(void)
{
    if (fd > 0)
        close(fd);
}

/*virtual*/ WebSocketRet
WebSocketConn :: handle_read(::google::protobuf::Message &msg)
{
    msg.Clear();
    if (readbuf.remaining() == 0)
    {
        cerr << "readbuffer is full!\n";
        return WEBSOCKET_NO_MESSAGE;
    }
    WebSocketRet r = handle_data(msg);
    if (r == WEBSOCKET_NO_MESSAGE)
    {
        int cc = readbuf.readFd(fd);
        if (cc < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                cerr << "read : " << strerror(errno) << endl;
                return WEBSOCKET_CLOSED;
            }
        }
        if (cc == 0)
        {
            // if we're out of data on the socket,
            // and there wasn't enough in the buffer
            // for a message, before, we're done.
            return WEBSOCKET_CLOSED;
        }
        r = handle_data(msg);
    }
    return r;
}

WebSocketRet
WebSocketConn :: handle_data(::google::protobuf::Message &msg)
{
    switch (state)
    {
    case STATE_HEADER:
        return handle_header();
    case STATE_CONNECTED:
        return handle_message(msg);
    }
    return WEBSOCKET_NO_MESSAGE;
}

WebSocketRet
WebSocketConn :: handle_message(::google::protobuf::Message &msg)
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

        if (server)
        {
            if ((readbuf[1] & 0x80) == 0)
            {
                cerr << "MASK=0 found, illegal for client->server" << endl;
                return WEBSOCKET_CLOSED;
            }
            header_len += 4; // account for mask
        }
        else
        {
            if ((readbuf[1] & 0x80) != 0)
            {
                cerr << "MASK=1 found, illegal for server->client" << endl;
                return WEBSOCKET_CLOSED;
            }
        }

        decoded_length = readbuf[1] & 0x7F;
        pos=2;

        // check if there's enough for a full message
        // given what we know so far.
        if (readbuf_len < (decoded_length+header_len))
        {
            if (verbose)
                cout << "handle_message bail out case 1" << endl;
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
                cout << "handle_message bail out case 2" << endl;
            // still not enough
            return WEBSOCKET_NO_MESSAGE;
        }

        int opcode = (readbuf[0] & 0xf);
        if (opcode != 2) // WS_TYPE_BINARY
        {
            if (opcode != 8) // WS_TYPE_CLOSE
                fprintf(stderr, "unhandled websocket opcode %d received\n",
                        opcode);
            return WEBSOCKET_CLOSED;
        }

        if (server)
        {
            uint8_t mask[4];
            uint32_t counter;
            for (counter = 0; counter < 4; counter++)
                mask[counter] = readbuf[pos + counter];
            pos += 4;

            for (counter = 0; counter < decoded_length; counter++)
                readbuf[pos + counter] =
                    readbuf[pos + counter] ^ mask[counter&3];
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
        cout << "handle_message bail out case 3" << endl;
    return WEBSOCKET_NO_MESSAGE;
}

bool
WebSocketConn::sendMessage(const ::google::protobuf::Message &msg)
{
    if (state != STATE_CONNECTED)
        return false;

    send_buffer.resize(0);

    send_buffer += (char) 0x82; // WS_TYPE_BINARY
//  send_buffer += (char) 0x88; // WS_TYPE_CLOSE

    int len = (int) msg.ByteSizeLong();
    int mask_bit = server ? 0 : 0x80;

    if (len < 126)
    {
        send_buffer += (char) ((len & 0x7f) | mask_bit);
    }
    else if (len < 65536)
    {
        send_buffer += (char) (126 | mask_bit); // mask bit
        send_buffer += (char) ((len >> 8) & 0xFF);
        send_buffer += (char) ((len >> 0) & 0xFF);
    }
    else
    {
        send_buffer += (char) (127 | mask_bit); // mask bit
        send_buffer += (char) 0;
        send_buffer += (char) 0;
        send_buffer += (char) 0;
        send_buffer += (char) 0;
        send_buffer += (char) ((len >> 24) & 0xFF);
        send_buffer += (char) ((len >> 16) & 0xFF);
        send_buffer += (char) ((len >>  8) & 0xFF);
        send_buffer += (char) ((len >>  0) & 0xFF);
    }

    uint8_t mask[4] = { 0, 0, 0, 0 };

    if (!server)
    {
        uint32_t mask_value = random();

        mask[0] = (mask_value >> 24) & 0xFF;
        mask[1] = (mask_value >> 16) & 0xFF;
        mask[2] = (mask_value >>  8) & 0xFF;
        mask[3] = (mask_value >>  0) & 0xFF;

        send_buffer += (char) mask[0];
        send_buffer += (char) mask[1];
        send_buffer += (char) mask[2];
        send_buffer += (char) mask[3];
    }

    size_t  pos = send_buffer.size();

    if (msg.AppendToString(&send_buffer) == false)
        return false;

    if (!server)
        for (size_t counter = 0; counter < len; counter++)
            send_buffer[pos + counter] =
                send_buffer[pos + counter] ^ mask[counter&3];

    const uint8_t * buf = (const uint8_t *) send_buffer.c_str();
    // repurpose len
    len = send_buffer.size();

    if (verbose)
    {
        printf("** write buffer : ");
        for (int ctr = 0; ctr < len; ctr++)
            printf("%02x ", buf[ctr]);
        printf("\n");
    }

    int cc = ::write(fd, buf, len);
    if (cc != len)
    {
        fprintf(stderr, "WebSocketClient :: sendMessage: "
                "write %d returned %d (err %d: %s)\n",
                len, cc, errno, strerror(errno));
        return false;
    }

    return true;
}

};
