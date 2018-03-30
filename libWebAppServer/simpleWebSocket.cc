
#include "simpleWebSocket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <regex.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include <ostream>
#include <iostream>
#include <iomanip>

#define VERBOSE 1

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

WebSocketConn :: WebSocketConn(int _fd, bool _server)
    : _ok(false), fd(_fd), server(_server), readbuf(MAX_READBUF)
{
    got_flags = 0;
}

WebSocketConn :: ~WebSocketConn(void)
{
    if (fd > 0)
        close(fd);
}

void
WebSocketConn :: set_nonblock(void)
{
    fcntl(fd, F_SETFL,
          fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void
WebSocketConn :: set_block(void)
{
    fcntl(fd, F_SETFL,
          fcntl(fd, F_GETFL) & ~O_NONBLOCK);
}

/*virtual*/ WebSocketRet
WebSocketConn :: handle_read(::google::protobuf::Message &msg)
{
    if (readbuf.remaining() == 0)
    {
        cerr << "readbuffer is full!\n";
        return WEBSOCKET_NO_MESSAGE;
    }

    set_nonblock();
    int cc = readbuf.readFd(fd);
    set_block();
    if (cc < 0)
    {
        if (errno != EWOULDBLOCK)
        {
            cerr << "read : " << strerror(errno) << endl;
            return WEBSOCKET_CLOSED;
        }
    }
    if (cc == 0 && readbuf.size() == 0)
        return WEBSOCKET_CLOSED;
    return handle_data(msg);
}

bool
WebSocketConn::sendMessage(const ::google::protobuf::Message &msg)
{
    if (state != STATE_CONNECTED)
        return false;

    send_buffer.resize(0);

    send_buffer += (char) 0x82; // WS_TYPE_BINARY
//  send_buffer += (char) 0x88; // WS_TYPE_CLOSE

    int len = msg.ByteSize();
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

    if (VERBOSE)
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

////////    WebSocketServer   ////////

WebSocketServer :: WebSocketServer( uint16_t port, uint32_t addr )
    : _ok(false), fd(-1)
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
    return new WebSocketServerConn(new_fd, sa);
}

};
