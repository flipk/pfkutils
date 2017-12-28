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

namespace ProtoSSL {

typedef google::protobuf::Message MESSAGE;
class ProtoSSLMsgs;

/*************************** ProtoSSLCertParams ***************************/

struct ProtoSSLCertParams
{
    const std::string &caCert;
    const std::string &myCert;
    const std::string &myKey;
    const std::string &myKeyPassword;
    ProtoSSLCertParams(const std::string &_caCert, // file:/...
                       const std::string &_myCert, // file:/...
                       const std::string &_myKey,  // file:/...
                       const std::string &_myKeyPassword);
    ~ProtoSSLCertParams(void);
};

/*************************** ProtoSSLConnClient ***************************/

class ProtoSSLConnClient;
class ProtoSSLConnClientHash;
typedef DLL3::List<ProtoSSLConnClient, 1/*uniqueID*/,
                   true/*lockWarn*/,true/*validate*/>  ClientList_t;
typedef DLL3::Hash<ProtoSSLConnClient, int/*fd*/,
                   ProtoSSLConnClientHash, 2/*uniqueID*/,
                   true/*lockWarn*/,true/*validate*/>  ClientHash_t;

struct ProtoSSLPeerInfo
{
    std::string ipaddr;
    std::string common_name;
    std::string pkcs9_email;
    std::string org_unit;
};

class ProtoSSLConnClient : public ClientList_t::Links,
                           public ClientHash_t::Links
{
    friend class ProtoSSLMsgs;
    friend class ProtoSSLConnClientHash;
    friend class ProtoSSLConnServer;
    bool _ok;
    bool send_close_notify;
    bool ssl_initialized;
    mbedtls_net_context netctx;
    mbedtls_ssl_context sslctx;
    pxfe_string rcvbuf;
    pxfe_string outbuf;
    ProtoSSLMsgs * msgs;
    // private so only ProtoSSLMsgs can make it.
    ProtoSSLConnClient(ProtoSSLMsgs * _msgs, mbedtls_net_context new_netctx);
    ProtoSSLConnClient(ProtoSSLMsgs * _msgs,
                       const std::string &remoteHost, int remotePort);
    bool init_common(void); // returns ok
    WaitUtil::Lockable ssl_lock;
public:
    virtual ~ProtoSSLConnClient(void);
    int get_fd(void) const { return netctx.fd; };
    enum read_return_t {
        GOT_DISCONNECT,
        READ_MORE,
        GOT_MESSAGE
    };
    read_return_t handle_read(MESSAGE &msg);
    // returns true if ok, false if not
    bool send_message(const MESSAGE &msg);
    bool ok(void) const { return _ok; }
    bool get_peer_info(ProtoSSLPeerInfo &info);
};
class ProtoSSLConnClientHash {
public:
    static uint32_t obj2hash  (const ProtoSSLConnClient &obj)
    { return obj.get_fd(); }
    static uint32_t key2hash  (const int fd) { return fd; }
    static bool     hashMatch (const int fd, const ProtoSSLConnClient &obj)
    { return fd == obj.get_fd(); }
};

/*************************** ProtoSSLConnServer ***************************/

class ProtoSSLConnServer;
class ProtoSSLConnServerHash;
typedef DLL3::List<ProtoSSLConnServer,/*uniqueID*/3,
                   true/*lockWarn*/,true/*validate*/>  ServerList_t;
typedef DLL3::Hash<ProtoSSLConnServer, int/*fd*/,
                   ProtoSSLConnServerHash, 4/*uniqueID*/,
                   true/*lockWarn*/,true/*validate*/>  ServerHash_t;

class ProtoSSLConnServer : public ServerList_t::Links,
                           public ServerHash_t::Links
{
    friend class ProtoSSLMsgs;
    friend class ProtoSSLConnServerHash;
    bool _ok;
    mbedtls_net_context netctx;
    ProtoSSLMsgs * msgs;
    // private so only ProtoSSLMsgs can make it.
    ProtoSSLConnServer(ProtoSSLMsgs * _msgs, int listeningPort);
public:
    virtual ~ProtoSSLConnServer(void);
    int get_fd(void) const { return netctx.fd; };
    // returns NULL if accept failed for some reason
    ProtoSSLConnClient * handle_accept(void);
    bool ok(void) const { return _ok; }
};
class ProtoSSLConnServerHash {
public:
    static uint32_t obj2hash  (const ProtoSSLConnServer &obj)
    { return obj.get_fd(); }
    static uint32_t key2hash  (const int fd) { return fd; }
    static bool     hashMatch (const int fd, const ProtoSSLConnServer &obj)
    { return fd == obj.get_fd(); }
};

/*************************** ProtoSSLMsgs ***************************/

class ProtoSSLMsgs
{
    friend class ProtoSSLConnServer;
    friend class ProtoSSLConnClient;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt         cacert, mycert;
    mbedtls_pk_context       mykey;
    mbedtls_ssl_config       sslcfg;
    bool                     nonBlockingMode;
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
    ProtoSSLMsgs(bool _nonBlockingMode, bool _debugFlag = false );
    ~ProtoSSLMsgs(void);
    // returns true if it could load, false if error
    bool loadCertificates(const ProtoSSLCertParams &params);
    ProtoSSLConnServer * startServer(int listeningPort);
    ProtoSSLConnClient * startClient(const std::string &remoteHost,
                                     int remotePort);
};

}; // namespace ProtoSSL

#endif /* __LIBPROTOSSL2_H__ */
