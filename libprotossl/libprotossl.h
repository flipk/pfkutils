/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __LIBPROTOSSL2_H__
#define __LIBPROTOSSL2_H__

#include "pfkutils_config.h"
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cookie.h>
#include <mbedtls/timing.h>
#if HAVE_MBEDTLS_NET_H
#include <mbedtls/net.h>
#endif
#if HAVE_MBEDTLS_NET_SOCKETS_H
#include <mbedtls/net_sockets.h>
#endif
#include <mbedtls/error.h>
#include <mbedtls/debug.h>
#include <google/protobuf/message.h>

#if !defined(MBEDTLS_SSL_PROTO_DTLS)
#error "DTLS not supported by this build of MBEDTLS ?"
#endif

#include "thread_slinger.h"
#include "posix_fe.h"
#include "dll3.h"
#include <map>
#include <deque>

namespace ProtoSSL {
    namespace DTLS {
        class DTLS_PacketHeader_m; // forward
    };
};

namespace ProtoSSL {

typedef google::protobuf::Message MESSAGE;
class ProtoSSLMsgs;

/*************************** ProtoSSLCertParams ***************************/

struct ProtoSSLCertParams
{
    const std::string caCert;
    const std::string myCert;
    const std::string myKey;
    const std::string myKeyPassword;
    ProtoSSLCertParams(const std::string &_caCert, // file:/...
                       const std::string &_myCert, // file:/...
                       const std::string &_myKey,  // file:/...
                       const std::string &_myKeyPassword);
    ~ProtoSSLCertParams(void);
};

/*************************** ProtoSSLConnClient ***************************/

class ProtoSSLConnClient;
typedef DLL3::List<ProtoSSLConnClient, 1/*uniqueID*/,
                   true/*lockWarn*/,true/*validate*/>  ClientList_t;

struct ProtoSSLPeerInfo
{
    std::string ipaddr;
    std::string common_name;
    std::string pkcs9_email;
    std::string org_unit;
};

class ProtoSSLConnClient : public ClientList_t::Links
{
    friend class ProtoSSLMsgs;
    friend class ProtoSSLConnServer;
    bool _ok;
    bool send_close_notify;
    bool ssl_initialized;
    bool net_initialized;
    mbedtls_net_context netctx;
    mbedtls_ssl_context sslctx;
    mbedtls_timing_delay_context timer;
    std::string rcvbuf;
    std::string outbuf;
    ProtoSSLMsgs * msgs;
    bool use_tcp;
    // private so only ProtoSSLMsgs can make it.
    ProtoSSLConnClient(ProtoSSLMsgs * _msgs, mbedtls_net_context new_netctx,
                       bool _use_tcp,
                       unsigned char *client_ip, size_t cliip_len);
    ProtoSSLConnClient(ProtoSSLMsgs * _msgs,
                       const std::string &remoteHost, int remotePort,
                       bool _use_tcp);
    bool init_common(unsigned char *client_ip, size_t cliip_len); // returns ok
    WaitUtil::Lockable ssl_lock;
public:
    static const uint32_t MAX_MSG_SIZE = MBEDTLS_SSL_MAX_CONTENT_LEN;

    virtual ~ProtoSSLConnClient(void);
    int get_fd(void) const { return netctx.fd; };
    enum read_return_t {
        GOT_DISCONNECT,
        READ_MORE,
        GOT_TIMEOUT,
        GOT_MESSAGE
    };

    // if you're using dtlsQ, do not use these two.
    read_return_t handle_read(MESSAGE &msg);
    bool send_message(const MESSAGE &msg);
    // these two are used by dtlsQ.
    read_return_t handle_read_raw(std::string &buffer);
    bool send_raw(const std::string &buffer);

    bool ok(void) const { return _ok; }
    bool get_peer_info(ProtoSSLPeerInfo &info);
};

/*************************** ProtoSSLConnServer ***************************/

class ProtoSSLConnServer;
typedef DLL3::List<ProtoSSLConnServer,/*uniqueID*/3,
                   true/*lockWarn*/,true/*validate*/>  ServerList_t;

class ProtoSSLConnServer : public ServerList_t::Links
{
    friend class ProtoSSLMsgs;
    bool _ok;
    mbedtls_net_context netctx;
    ProtoSSLMsgs * msgs;
    bool use_tcp;
    // private so only ProtoSSLMsgs can make it.
    ProtoSSLConnServer(ProtoSSLMsgs * _msgs, int listeningPort, bool _use_tcp);
public:
    ProtoSSLConnServer() = delete;
    virtual ~ProtoSSLConnServer(void);
    int get_fd(void) const { return netctx.fd; };
    // returns NULL if accept failed for some reason
    ProtoSSLConnClient * handle_accept(void);
    bool ok(void) const { return _ok; }
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
    mbedtls_ssl_cookie_ctx   cookie_ctx;
    bool                     nonBlockingMode;
    bool                     debugFlag;
    bool                     use_tcp;
    ClientList_t             clientList;
    ServerList_t             serverList;
    void   registerServer(ProtoSSLConnServer * svr);
    void unregisterServer(ProtoSSLConnServer * svr);
    void   registerClient(ProtoSSLConnClient * clnt);
    void unregisterClient(ProtoSSLConnClient * clnt);
public:
    // read_timeout is in milliseconds
    ProtoSSLMsgs(bool _nonBlockingMode, bool _debugFlag = false,
                 uint32_t read_timeout = 0, bool use_tcp = true,
                 uint32_t dtls_timeout_min = 1000,
                 uint32_t dtls_timeout_max = 2000 );
    ~ProtoSSLMsgs(void);
    // returns true if it could load, false if error
    bool loadCertificates(const ProtoSSLCertParams &params);
    // optional: check if the loaded certs actually validate
    bool validateCertificates(void);
    ProtoSSLConnServer * startServer(int listeningPort);
    ProtoSSLConnClient * startClient(const std::string &remoteHost,
                                     int remotePort);
};

struct ProtoSslDtlsQueueConfig
{
    ProtoSslDtlsQueueConfig(void)
    {
        // fill out defaults!
        fragment_size = default_fragment_size;
        window_size = default_window_size;
        ticks_per_second = default_ticks_per_second;
        hearbeat_interval = default_hearbeat_interval;
        max_missed_heartbeats = default_max_missed_heartbeats;
        max_outstanding_acks = default_max_outstanding_acks;
        max_outstanding_nacks = default_max_outstanding_nacks;
        num_queues = 0;
    }

    static const uint32_t default_fragment_size         = 1250; // 10^4 bits
    static const uint32_t default_window_size           = 1000;
    static const uint32_t default_ticks_per_second      = 25;
    static const uint32_t default_hearbeat_interval     = 25; // 1 second
    static const uint32_t default_max_missed_heartbeats = 3;  // 3 seconds
    static const uint32_t default_max_outstanding_acks  = 1;
    static const uint32_t default_max_outstanding_nacks = 1;

    static const uint32_t max_queues                    = 8;

    // technically, fragment_size is the size of the data portion of the
    // fragment messages NOT counting SSL overhead NOR counting
    // DTLS_PacketHeader_m overhead, so........ buyer beware.
    uint32_t fragment_size;
    uint32_t window_size;
    uint32_t ticks_per_second;
    uint32_t hearbeat_interval;
    // handle_read will return LINK_DOWN when
    // this many heartbeats are missed.
    uint32_t max_missed_heartbeats;
    // advantage to ack/nack packing: can ack several packets in a single
    // response. disadvantages: when a single fragment containing several acks
    // is lost, all of those frags get retransmitted, even the ones that got
    // through; and the acks aren't sent immediately. (if the max is set to 1,
    // the heartbeat message with piggybacked acks is forced immediately.)
    uint32_t max_outstanding_acks;
    uint32_t max_outstanding_nacks;

    enum Queue_Type_t { STACK, FIFO };
    enum Limit_Type_t { NONE, MESSAGES, BYTES };

    struct queue_config_t {
        bool          reliable;
        Queue_Type_t  qt;
        Limit_Type_t  lt;
        uint32_t      limit;
    };

private:
    // NOTE indexes returned by add_queue do NOT index
    //      this array! that's why this is private, to
    //      point the gun away from the user's foot.
    friend class ProtoSslDtlsQueue;
    queue_config_t queue_config[max_queues];
    uint32_t num_queues;

public:
    // this returns the queue_number, which will be 1-N
    bool add_queue(uint32_t &queue_number, bool reliable,
                   Queue_Type_t qt, Limit_Type_t lt, uint32_t limit)
    {
        if (num_queues >= max_queues)
            return false;
        queue_config[num_queues].reliable = reliable;
        queue_config[num_queues].qt = qt;
        queue_config[num_queues].lt = lt;
        queue_config[num_queues].limit = limit;
        queue_number = ++num_queues; // yes, this is correct.
        return true;
    }
    const queue_config_t &get_queue_config(uint32_t queue_number) const
    {
        // note queue_numbers are numbered 1-N
        if (queue_number > num_queues)
            // handle error?
            return queue_config[0];
        return queue_config[queue_number-1];
    }
};

struct ProtoSslDtlsQueueStatistics
{
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t frags_sent;
    uint64_t frags_received;
    uint64_t frags_resent;
    uint64_t missing_frags_detected;

    ProtoSslDtlsQueueStatistics(void) { init(); }
    void init(void)
    {
        bytes_sent = 0;
        bytes_received = 0;
        frags_sent = 0;
        frags_received = 0;
        frags_resent = 0;
        missing_frags_detected = 0;
    }
    std::string Format(void);
};

class ProtoSslDtlsQueue
{
    const ProtoSslDtlsQueueConfig config;
    ProtoSSLConnClient * client;
    WaitUtil::Lockable  dtls_lock;
    bool link_up;
    bool link_changed;
    bool _ok;
    ProtoSslDtlsQueueStatistics stats;

public:
    enum read_return_t {
        GOT_DISCONNECT,
        READ_MORE,
        GOT_TIMEOUT,
        GOT_MESSAGE,
        LINK_DOWN,
        LINK_UP
    };
    enum send_return_t {
        SEND_SUCCESS, // message is queued / on the way.
        MESSAGE_TOO_BIG,   // too big for window_size * fragment_size,
                           // or just fragment_size on unreliable queue.
        BOGUS_QUEUE_NUMBER, // queue number is bogusly bogus.
        MSG_NOT_INITIALIZED, // msg.IsInitialized() returned false.
        QOS_DROP, // configured queue has exceeded set limits; note this
                  // is returned for FIFO queues, but not STACK queues,
                  // because stacks drop from the other end.
        CONN_SHUTDOWN // you have called shutdown(), don't call send_message
    };

private:
    struct dtls_send_event : public ThreadSlinger::thread_slinger_message
    {
        enum { TICK, MSG, ACK, NACK, DIE } type;
        bool reliable; // used only for MSG
        uint32_t seqno; // used  by ACK/NACK; NOT valid for MSG
        std::string encoded_msg; // used by MSG
    };
    struct dtls_read_event : public ThreadSlinger::thread_slinger_message
    {
        read_return_t  retcode;
        std::string encoded_msg;
    };

    static const int pool_size = 1000;
    static const int other_pool_size = 100;
    // only type==MSG from msg_pool, rest from other_pool.
    // this prevents deadlocks when msg_pool is extinguished;
    // acks can still get through from recv_thread.
    ThreadSlinger::thread_slinger_pool<dtls_send_event>  send_msg_pool;
    ThreadSlinger::thread_slinger_pool<dtls_send_event>  send_other_pool;
    ThreadSlinger::thread_slinger_pool<dtls_read_event>  read_pool;

    void send_event_release(dtls_send_event *dte) {
        if (dte) {
            if (dte->type == dtls_send_event::MSG)
                send_msg_pool.release(dte);
            else
                send_other_pool.release(dte);
        }
    }

    typedef ThreadSlinger::thread_slinger_queue<dtls_send_event> dtls_thread_queue_t;
    typedef ThreadSlinger::thread_slinger_queue<dtls_read_event> dtls_read_queue_t;
    dtls_thread_queue_t  send_q;
    dtls_read_queue_t    recv_q;

    // this is always config.num_queues + 1, because
    // queues[0] is always send_q. also note that
    // Config::add_queue always returns 1-N so we can just
    // use queues[queue_number].
    uint32_t num_queues;
    dtls_thread_queue_t  * queues[ProtoSslDtlsQueueConfig::max_queues];
    uint32_t          queue_sizes[ProtoSslDtlsQueueConfig::max_queues]; // bytes

    // thread maintenance junk
    // 0 = tick_thread, 1 = recv_thread, 2 = send_thread.
    static const uint32_t num_threads = 3;
    pthread_t thread_ids[num_threads];
    bool thread_started[num_threads];
    uint32_t startWhich;
    WaitUtil::Waitable  startThreadSync;
    bool start_thread(uint32_t which, pthread_t *);
    static void *_start_thread(void *);
    pxfe_pipe closer_pipe;

    // tick thread
    void tick_thread(void);
    uint32_t  tick; // for debug, don't depend on it, due to wrappage.

    // send thread
    void send_thread(void);
    void handle_tick(void);
    // note queue_number=-1 means we're trying again on an ondeck
    // message, so we don't have to account for it again in queue_sizes.
    // false ret means send window is full, couldn't send.
    bool handle_send_msg(dtls_send_event *dte, int queue_number = -1);
    void handle_nack(dtls_send_event *dte);
    void send_heartbeat(void) { WaitUtil::Lock l(&dtls_lock); send_frag(NULL); }

    struct dtls_fragment : public ThreadSlinger::thread_slinger_message
    {
        dtls_fragment(void);
        ~dtls_fragment(void);
        void init();
        int refcount;
        uint32_t seqno; // non-modulo'd original seqno.
        bool delivered; // used to detect lost ack and prevent double-delivery
        uint32_t age; // set to 0 on entry to send_window.
        std::string fragment; // body, not header
        ProtoSSL :: DTLS :: DTLS_PacketHeader_m  *pkthdr;
        // debug
        std::string print(void) const;
    };

    struct fragpool_t {
    private:
        ThreadSlinger::thread_slinger_pool<dtls_fragment> fragpool;
    public:
        // alloc from fragpool first; if empty, new.
        inline dtls_fragment *alloc(void); // note frag->refcount == 1
        inline void deref(dtls_fragment *);
    } fragpool;

    // fragsender. if we have an ack or nack to send, but
    // don't want to send them immediately, we can wait until
    // we have something else to send-- put them here, on deck.
    std::deque<uint32_t> ondeck_ack_seq_nos;
    std::deque<uint32_t> ondeck_nack_seq_nos;
    void push_ondeck_acks(uint32_t seqno);
    std::string frag_send_buffer; // reuse, to prevent excessive reallocs
    // invoke with NULL to just send heartbeat, acknacks.
    void send_frag(dtls_fragment *, const char *reason = NULL);

    // this is the next sequence number we're going to send.
    // the position in send_window where it belongs is
    // "send_seqno % config.window_size"
    uint32_t send_seqno;
    std::vector<dtls_fragment*>   send_window;

    // for calculating retransmit age.
    uint64_t  rtd_rsp_timestamp;
    time_t last_rtd_req_time;
    uint32_t retransmit_age; // in ticks

    // recv thread
    void recv_thread(void);
    void handle_got_frag(void);
    // returns number of frags consumed from recv_reassembly;
    uint32_t recv_reassemble_deliver(uint32_t seqno);
    void handle_recv_ack(uint32_t recv_seq_no);
    void handle_recv_nack(uint32_t recv_seq_no);

    // next seqno to be received, assembled, and delivered
    // to application.
    uint32_t recv_seqno;
    std::string frag_recv_buffer; // reuse, to prevent excessive reallocs
    std::vector<dtls_fragment *> recv_frag_list; // reuse to prevent reallocs

    // see the big comment in got_frag to see how these are used.
    std::vector<dtls_fragment*>   recv_window;
    typedef std::map<uint32_t/*seqno*/,dtls_fragment*>  ReassemblyMap;
    ReassemblyMap recv_reassembly;

    uint32_t ticks_without_recv; // trigger for LINK_DOWN
    uint32_t ticks_without_send; // trigger for hearbeat

public:
    // provide a connected SSLConnClient object, and this object
    // takes over ownership of it. when this class is deleted,
    // the SSL client will be deleted (connection closed) too.
    ProtoSslDtlsQueue (const ProtoSslDtlsQueueConfig &_config,
                       ProtoSSLConnClient *);

    // call this after construction to see if construction was okay.
    bool ok(void) const { return _ok; }

    // if you didn't call shutdown, this will do it for you. it will
    // also delete the SSLConnClient for you.
    ~ProtoSslDtlsQueue(void);

    void get_stats(ProtoSslDtlsQueueStatistics *stats);

    send_return_t send_message(uint32_t queue_number, const MESSAGE &);

    // don't use the ConnClient's get_fd to select. just call this and
    // it blocks until something happens. if you delete this object
    // from another thread, you'll get a DISCONNECT from this call.
    // sorry i know this means you can't do multiple descriptors or
    // multiple DtlsQ's in one thread, but there's reasons why this is.
    // if we're truly desperate for that in the future, we could
    // expose the recv_q to the caller so the caller could do a
    // multi_dequeue on all of them.
    read_return_t handle_read(MESSAGE &msg);

    // shut down the dtlsq connection. if you've got a thread blocked
    // in handle_read, it will wake up immediately with GOT_DISCONNECT.
    // it will also close the SSLConnClient. at this point the DtlsQ
    // is dead and there is nothing left for it but to be deleted.
    // NOTE you don't have to call shutdown, as the destructor will
    // do the shutdown; but having a separate shutdown method is useful
    // if you want to control when the object gets deleted, or your
    // thread calling handle_read can't be guaranteed to not access
    // the pointer after deletion.
    void shutdown(void);
};

}; // namespace ProtoSSL

#endif /* __LIBPROTOSSL2_H__ */
