/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __LIBPROTOSSL2_H__
#define __LIBPROTOSSL2_H__

#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <mbedtls/net.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>
#include <google/protobuf/message.h>

#include "posix_fe.h"
#include "dll3.h"

// namespace ProtoSSL {

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

class ProtoSSLMsgs;
class ProtoSSLConnClient;
class ProtoSSLConnClientHash;
class ProtoSSLConnServer;
class ProtoSSLConnServerHash;

typedef DLL3::List<ProtoSSLConnClient, 1/*uniqueID*/,
                   false/*lockWarn*/,false/*validate*/>  ClientList_t;
typedef DLL3::Hash<ProtoSSLConnClient, int/*fd*/,
                   ProtoSSLConnClientHash, 2/*uniqueID*/,
                   false/*lockWarn*/,false/*validate*/>  ClientHash_t;

typedef DLL3::List<ProtoSSLConnServer,/*uniqueID*/3,
                   false/*lockWarn*/,false/*validate*/>  ServerList_t;
typedef DLL3::Hash<ProtoSSLConnServer, int/*fd*/,
                   ProtoSSLConnServerHash, 4/*uniqueID*/,
                   false/*lockWarn*/,false/*validate*/>  ServerHash_t;

class ProtoSSLConnClient : public ClientList_t::Links,
                           public ClientHash_t::Links
{
    friend class ProtoSSLMsgs;
    friend class ProtoSSLConnClientHash;
    mbedtls_net_context netctx;
    mbedtls_ssl_context sslctx;
    // private so only ProtoSSLMsgs can make it.
    int fd;
    ProtoSSLMsgs * msgs;
    ProtoSSLConnClient(ProtoSSLMsgs * _msgs);
public:
    virtual ~ProtoSSLConnClient(void);
    int get_fd(void) const { return fd; };
    enum read_return_t {
        GOT_CONNECT,
        GOT_DISCONNECT,
        READ_MORE,
        GOT_MESSAGE
    };
    read_return_t handle_read(MESSAGE &msg);
    // returns true if ok, false if not
    bool send_message(const MESSAGE &msg);
};
class ProtoSSLConnClientHash {
public:
    static uint32_t obj2hash  (const ProtoSSLConnClient &obj) {
        return obj.fd;
    }
    static uint32_t key2hash  (const int fd) {
        return fd;
    }
    static bool     hashMatch (const int fd, const ProtoSSLConnClient &obj) {
        return fd == obj.fd;
    }
};

class ProtoSSLConnServer : public ServerList_t::Links,
                           public ServerHash_t::Links
{
    friend class ProtoSSLMsgs;
    friend class ProtoSSLConnServerHash;
    // private so only ProtoSSLMsgs can make it.
    int fd;
    ProtoSSLMsgs * msgs;
    ProtoSSLConnServer(ProtoSSLMsgs * _msgs);
public:
    virtual ~ProtoSSLConnServer(void);
    int get_fd(void) const { return fd; };
    // returns NULL if accept failed for some reason
    ProtoSSLConnClient * handle_accept(void);
};
class ProtoSSLConnServerHash {
public:
    static uint32_t obj2hash  (const ProtoSSLConnServer &obj) {
        return obj.fd;
    }
    static uint32_t key2hash  (const int fd) {
        return fd;
    }
    static bool     hashMatch (const int fd, const ProtoSSLConnServer &obj) {
        return fd == obj.fd;
    }
};

class ProtoSSLMsgs
{
    friend class ProtoSSLConnServer;
    friend class ProtoSSLConnClient;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt         cacert, mycert;
    mbedtls_pk_context       mykey;
    mbedtls_ssl_config       sslcfg;
    bool                     debugFlag;
    ClientList_t             clientList;
    ClientHash_t             clientHash;
    ServerList_t             serverList;
    ServerHash_t             serverHash;
    void   registerServer(ProtoSSLConnServer * svr);
    void unregisterServer(ProtoSSLConnServer * svr);
    void   registerClient(ProtoSSLConnClient * clnt);
    void unregisterClient(ProtoSSLConnClient * clnt);
public:
    ProtoSSLMsgs(bool _debugFlag = false);
    ~ProtoSSLMsgs(void);
    // returns true if it could load, false if error
    bool loadCertificates(const ProtoSSLCertParams &params);
    ProtoSSLConnServer * startServer(int listeningPort);
    ProtoSSLConnClient * startClient(const std::string &remoteHost,
                                     int remotePort);
};

// }; // namespace ProtoSSL

#endif /* __LIBPROTOSSL2_H__ */
