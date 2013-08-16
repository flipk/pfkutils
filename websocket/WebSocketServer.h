
/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

#ifndef __WEBSOCKETSERVER_H__
#define __WEBSOCKETSERVER_H__

#include <unistd.h>
#include <stdint.h>

enum WebSocketMessageType { 
    WS_TYPE_TEXT,
    WS_TYPE_BINARY,
    WS_TYPE_CLOSE
};

struct WebSocketMessage {
    WebSocketMessageType type;
    uint8_t * buf;
    int len;
};

class WebSocketConnection {
    friend class WebSocketServer;
    void connection_thread_main(void);
    void handle_header(void);
    void send_handshake_response(void);
    void handle_message(void);
    int fd;
    static const int maxbufsize = 8192;
    uint8_t buf[maxbufsize+1];
    int bufsize;
    enum { STATE_HEADER, STATE_CONNECTED } state;
    bool done;
    char * host;
    char * key;
    char * origin;
    char * version;
    char * resource;
    bool upgrade_flag;
    bool connection_flag;
public:
    WebSocketConnection(int _fd);
    virtual ~WebSocketConnection(void);
    virtual void onMessage(const WebSocketMessage &m) = 0;
    void sendMessage(const WebSocketMessage &m);
};

class WebSocketConnectionCallback {
public:
    virtual WebSocketConnection * newConnection(int fd) = 0;
};

class WebSocketServer {
    int wsfd;
    int closePipe[2];
    WebSocketConnectionCallback *cb;
    static void * connection_thread_entry( void * arg );
    static void * server_thread_entry( void * arg );
    void server_thread_main( void );
    bool threadRunning;
public:
    WebSocketServer(void);
    ~WebSocketServer(void);
    // return false if failure to start
    bool start(int port, WebSocketConnectionCallback *);
    void stop(void);
};

#endif /* __WEBSOCKETSERVER_H__ */
