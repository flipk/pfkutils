/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __LIBPROTOSSL_H__
#define __LIBPROTOSSL_H__

#include <string>
#include <google/protobuf/message.h>
#include <polarssl/entropy.h>
#include <polarssl/ctr_drbg.h>
#include <polarssl/ssl.h>
#include <polarssl/net.h>

namespace ProtoSSL {

enum ProtoSSLEventType {
    PROTOSSL_CONNECT,
    PROTOSSL_DISCONNECT,
    PROTOSSL_TIMEOUT,
    PROTOSSL_MESSAGE,
    PROTOSSL_RETRY
};

template <class IncomingMessageType>
struct ProtoSSLEvent {
    ProtoSSLEventType type;
    int connectionId;
    IncomingMessageType * msg; // null if connect or disconnect
};

struct ProtoSSLCertParams {
    const std::string &caCertFile;
    const std::string &myCertFile;
    const std::string &myKeyFile;
    const std::string &myKeyPassword;
    const std::string &otherCommonName;
    ProtoSSLCertParams(const std::string &_caCertFile,
                       const std::string &_myCertFile,
                       const std::string &_myKeyFile,
                       const std::string &_myKeyPassword,
                       const std::string &_otherCommonName);
    ~ProtoSSLCertParams(void);
};

class __ProtoSSLMsgs {
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    x509_crt cacert, mycert;
    pk_context   mykey;
    ssl_context  sslctx;
    std::string  otherCommonName;
    std::string  remoteHost;
    int port;
    bool isServer;
    bool good;
    enum {
        WAIT_FOR_CONN, // server
        TRY_CONN, // client
        SERVER_CONNECTED,
        CLIENT_CONNECTED
    } state;
    int listen_fd;
    int fd;
    std::string rcvbuf;
    std::string outbuf;
    google::protobuf::Message * rcvdMsg;
    bool initCommon(void);
    bool loadCertificates(const ProtoSSLCertParams &params);
    bool handShakeCommon(void);
protected:
    void setRcvdMsg(google::protobuf::Message * msg);
    ProtoSSLEventType _getEvent(int &connectionId, int timeoutMs);
    google::protobuf::Message * _getMsgPtr(void);
    bool _sendMessage(const google::protobuf::Message &msg);
public:
    __ProtoSSLMsgs(const ProtoSSLCertParams &params,
                   int listeningPort);
    __ProtoSSLMsgs(const ProtoSSLCertParams &params,
                   const std::string &remoteHost, int remotePort);
    virtual ~__ProtoSSLMsgs(void);
    bool isGood(void) { return good; }
};

template <class IncomingMessageType, class OutgoingMessageType>
class ProtoSSLMsgs : public __ProtoSSLMsgs {
    IncomingMessageType _rcvdMsg;
public:
    ProtoSSLMsgs(const ProtoSSLCertParams &params, int listeningPort);
    ProtoSSLMsgs(const ProtoSSLCertParams &params,
                 const std::string &remoteHost, int remotePort);
    virtual ~ProtoSSLMsgs(void);
    void getEvent( ProtoSSLEvent<IncomingMessageType> &event, int timeoutMs );
    bool sendMessage(const OutgoingMessageType &msg);
};

#include "libprotossl.tcc"

}; // namespace ProtoSSL

#endif /* __LIBPROTOSSL_H__ */
