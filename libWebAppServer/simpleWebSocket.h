/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <string>
#include <ostream>
#include <netinet/in.h>
#include <google/protobuf/message.h>
#include "CircularReader.h"

namespace SimpleWebSocket {

class Url {
    bool _ok;
public:
    std::string url;
    std::string protocol; // http, https, ws, wss
    std::string hostname; // or ip addr
    uint32_t addr; // if hostname could be resolved, or -1 if not
    uint16_t port;
    std::string path;
    Url(void);
    Url(const std::string &url);
    ~Url(void);
    bool ok(void) const { return _ok; }
    bool parse(const std::string &url);
};

extern std::ostream &operator<<(std::ostream &str, const Url &u);

enum WebSocketRet {
    WEBSOCKET_CONNECTED,   // should call get_path now
    WEBSOCKET_NO_MESSAGE,  // incomplete msg; go back to select, check later
    WEBSOCKET_MESSAGE,     // consume msg
    WEBSOCKET_CLOSED       // should delete me now
};

class WebSocketConn {
protected:
    bool _ok; // not ok until negotiation complete.
    int fd;
    bool server;
    enum {
        STATE_HEADER,   // waiting for ws mime headers
        STATE_CONNECTED // mime headers done, exchanging messages
    } state;
    int got_flags;
    bool verbose;
    std::string send_buffer;
    WebSocketConn( int fd, bool server, bool _verbose );
    static const int MAX_READBUF = 65536 + 0x1000; // 65K
    CircularReader  readbuf;
    virtual WebSocketRet handle_data(::google::protobuf::Message &msg) = 0;
public:
    virtual ~WebSocketConn(void);
    // returns the file descriptor used by this conn
    int get_fd(void) const { return fd; }
    // returns false if not connected/negotiated
    bool ok(void) const { return _ok; }
    // call this whenever fd is ready. initially, negotiation is
    // in progress and cannot be used to transfer messages.
    // if this returns CONNECTED, negotiation is now complete and
    //   this connection can be used to pass messages.
    // if this returns NO_MESSAGE, data may have been passed but a
    //   websocket message was not completed or negotiation is still
    //   in progress.
    // if this returns MESSAGE, you should consume msg.
    // if this returns CLOSED, the connection is gone, for whatever
    //    reason; also, this object is now dead, delete it.
    //   (if it returns CLOSED without ever returning CONNECTED,
    //    that means negotiation failed.)
    WebSocketRet handle_read(::google::protobuf::Message &msg);
    // returns false if there was a problem sending the message;
    // should call ok() to see if connection is now dead.
    bool sendMessage(const ::google::protobuf::Message &msg);
};

class WebSocketServerConn : public WebSocketConn {
    std::string path; // not valid until _ok
    struct sockaddr_in sa;
    static const int GOT_NONE            = 0;
    static const int GOT_HOST            = 1;
    static const int GOT_KEY             = 2;
    static const int GOT_ORIGIN          = 4;
    static const int GOT_VERSION         = 8;
    static const int GOT_RESOURCE        = 16;
    static const int GOT_UPGRADE_FLAG    = 32;
    static const int GOT_CONNECTION_FLAG = 64;
    static const int GOT_ALL             = 127;  // all combined
    std::string host;
    std::string origin;
    std::string version;
    std::string key;
    /*virtual*/ WebSocketRet handle_data(::google::protobuf::Message &msg);
    WebSocketRet handle_header(void);
    bool handle_header_line(const CircularReaderSubstr &headerLine);
    WebSocketRet handle_message(::google::protobuf::Message &msg);
    void send_handshake_response(void);
public:
    // this constructor invoked by WebSocketServer upon new connection.
    WebSocketServerConn(int _fd, const struct sockaddr_in &_sa,
                        bool _verbose = false);
    /*virtual*/ ~WebSocketServerConn(void);
    bool get_path(std::string &_path) const {
        if (_ok)
            _path = path;
        return _ok;
    }
};

class WebSocketClientConn : public WebSocketConn {
    static const int  GOT_NONE               = 0;
    static const int  GOT_SWITCHING          = 1;
    static const int  GOT_CONNECTION_UPGRADE = 2;
    static const int  GOT_UPGRADE_WEBSOCKET  = 4;
    static const int  GOT_ACCEPT             = 8;
    static const int  GOT_ALL                = 15; // all combined
    uint32_t urlIp;
    uint16_t urlPort;
    std::string urlHost;
    std::string urlPath;
    std::string secWebsocketKey;
    std::string secWebsocketKeyResponse;
    void generateWsHeaders(std::ostringstream &hdrs);
    const std::string hostForConn(void) const;
    void init_common(void);
    /*virtual*/ WebSocketRet handle_data(::google::protobuf::Message &msg);
    WebSocketRet handle_wsheader(const CircularReaderSubstr &hdr);
    WebSocketRet handle_message(::google::protobuf::Message &msg);
public:
    // check _ok to see if construction succeeded; check errno if not.
    // use this version for address, port, and path explicitely specified.
    WebSocketClientConn(uint32_t addr, uint16_t port,
                        const std::string &path, bool _verbose = false);
    // use this version for a URL "ws://host:port/path"
    // note this version does not support web proxies, although WebAppServer
    // version does, so if it was needed, it could be easily ported.
    WebSocketClientConn(const std::string &url, bool _verbose = false);
    /*virtual*/ ~WebSocketClientConn(void);
};

class WebSocketServer {
    bool _ok;
    int fd;
    bool verbose;
public:
    // check _ok to see if port setup succeeded; check errno if not.
    WebSocketServer(uint16_t bind_port, uint32_t bind_addr = INADDR_ANY,
                    bool _verbose = false);
    ~WebSocketServer(void);
    int get_fd(void) const { return fd; }
    // returns NULL if accept failed for some reason; check errno.
    WebSocketServerConn * handle_accept(void);
    bool ok(void) const { return _ok; }
};

extern const std::string websocket_guid;

}; // namespace SimpleWebSocket
