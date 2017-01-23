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

#ifndef __LIBPROTOSSL_H__
#define __LIBPROTOSSL_H__

#include <string>
#include <map>
#include <pthread.h>
#include <unistd.h>

#include <google/protobuf/message.h>
#include <polarssl/entropy.h>
#include <polarssl/ctr_drbg.h>
#include <polarssl/ssl.h>
#include <polarssl/net.h>

#include "LockWait.h"

namespace ProtoSSL {

typedef google::protobuf::Message MESSAGE;

struct ProtoSSLCertParams
{
    const std::string &caCert;
    const std::string &myCert;
    const std::string &myKey;
    const std::string &myKeyPassword;
    const std::string &otherCommonName;
    ProtoSSLCertParams(const std::string &_caCert, // file:/...
                       const std::string &_myCert, // file:/...
                       const std::string &_myKey,  // file:/...
                       const std::string &_myKeyPassword,
                       const std::string &_otherCommonName);
    ~ProtoSSLCertParams(void);
};

class ProtoSSLMsgs; // forward

class _ProtoSSLConn
{
    friend class ProtoSSLMsgs;
    int fd;
    WaitUtil::Lockable fdLock;
    std::string rcvbuf;
    std::string outbuf;
    ssl_context  sslctx;
    MESSAGE &rcvdMessage;
    static void * threadMain(void *);
    void _threadMain(void);
    pthread_t thread_id;
    bool thread_running;
    int exitPipe[2];
    // used by ProtoSSLMsgs, our friend.
    bool _startThread(ProtoSSLMsgs * _msgs, bool isServer, int _fd);
protected:
    _ProtoSSLConn(MESSAGE &_rcvdMessage);
    virtual ~_ProtoSSLConn(void);
    virtual bool _messageHandler(void) = 0;
    bool _sendMessage(MESSAGE &);
    ProtoSSLMsgs * msgs;
    // this tells the user the authentication is complete and the
    // connection is ready to pass encrypted protobuf messages.
    // TODO : connect could pass more information about the peer.
    virtual void handleConnect(void) = 0;
public:
    // the user may use this to stop all proto ssl messaging,
    // equivalent to calling ProtoSSLMsgs::stop.
    void stopMsgs(void);
    // the user may call this to cleanly close this connection.
    void closeConnection(void);
};

template <class IncomingMessageType, class OutgoingMessageType>
class ProtoSSLConn : public _ProtoSSLConn
{
    IncomingMessageType  _rcvdMessage;
    OutgoingMessageType  _outMessage;
    bool _messageHandler(void) { return messageHandler(_rcvdMessage); }
protected:
    // constructor indicates a new tcp connection but does NOT
    // indicate authentication.
    ProtoSSLConn(void) : _ProtoSSLConn(_rcvdMessage) { }
    // the user's handler should return false to kill the connection.
    virtual bool messageHandler(const IncomingMessageType &) = 0;
    // this indicates the connection is closed and thread is exiting.
    virtual ~ProtoSSLConn(void) { }
    OutgoingMessageType &outMessage(void) { return _outMessage; }
    // note after sending, this calls outMessage.Clear()
    bool sendMessage(void) { return _sendMessage(_outMessage); }
};

class ProtoSSLConnFactory
{
public:
    virtual ~ProtoSSLConnFactory(void) { }
    virtual _ProtoSSLConn * newConnection(void) = 0;
};

class ProtoSSLMsgs
{
    friend class _ProtoSSLConn;
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    x509_crt cacert, mycert;
    pk_context   mykey;
    std::string  otherCommonName;
    std::string  remoteHost;
    WaitUtil::Lockable connLock;
    typedef std::map<int/*fd*/,_ProtoSSLConn*> connMap;
    connMap conns;
    struct serverInfo {
        int fd;
        pthread_t thread_id;
        ProtoSSLMsgs * msgs;
        int exitPipe[2];
        ProtoSSLConnFactory *factory;
    };
    typedef std::map<int,serverInfo> serverInfoMap;
    serverInfoMap servers; // lock this with connMap too
    void deregisterConn(int fd,_ProtoSSLConn *);
    static void * serverThread(void *);
    void _serverThread(serverInfo *);
    int exitPipe[2];
public:
    ProtoSSLMsgs(void);
    ~ProtoSSLMsgs(void);
    bool loadCertificates(const ProtoSSLCertParams &params);
    bool startServer(ProtoSSLConnFactory &factory,
                     int listeningPort);
    bool startClient(ProtoSSLConnFactory &factory,
                     const std::string &remoteHost, int remotePort);
    // returns false if someone called 'stop',
    // returns true if timeout reached.
    bool run(int timeout_ms = -1);
    void stop(void);
};

}; // namespace ProtoSSL

#endif /* __LIBPROTOSSL_H__ */
