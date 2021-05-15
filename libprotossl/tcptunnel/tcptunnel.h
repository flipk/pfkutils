
#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>
#include <vector>
#include <string>
#include "libprotossl.h"
#include "posix_fe.h"
#include TCPTUNNEL_PB_H

class TcpTunnel {
public:
    TcpTunnel(int argc, char ** argv);
    ~TcpTunnel(void);
    bool ok(void) const { return _ok; }
    void usage(void);
    int main(void);

private:
    bool _ok;

    enum { SERVER_MODE, CLIENT_MODE } mode;
    uint32_t peer_ip; // only in client mode
    std::string peer_host;
    uint32_t peer_port;

    ProtoSSL::ProtoSSLMsgs * msgs;
    ProtoSSL::ProtoSslDtlsQueueConfig  dtlsqConfig;
    ProtoSSL::ProtoSslDtlsQueue * client_dtlsq;
    uint32_t dtlsq_num;

    struct proxy_info {
        uint32_t local_port;
        uint32_t remote_ip;
        uint32_t remote_port;
        // not opened until acceptorthread
        pxfe_tcp_stream_socket  sock;
    };
    std::vector<proxy_info> proxy_ports;

    struct proxy_conn_info {
        int proxy_index;
        pthread_t proxythread_id;
        pxfe_tcp_stream_socket * sock;
    };
    std::vector<proxy_conn_info> proxy_conns;

    pxfe_pipe  acceptor_closer_pipe;
    pthread_t clientthread_id;
    pthread_t acceptorthread_id;

    // tcptunnel_threadstarter.cc
    struct thread_args {
        TcpTunnel * tcptun;
        void (TcpTunnel::*func)(thread_args *);
    };
    void start_a_thread(pthread_t *id, thread_args *args);
    static void *threadstarter(void*);

    // tcptunnel_client_thread.cc
    void start_client_thread(ProtoSSL::ProtoSSLConnClient * client,
                             bool block);
    struct clientthread_args : public thread_args {
        ProtoSSL::ProtoSSLConnClient * client;
    };
    void clientthread(thread_args *args);

    // tcptunnel_proxy.cc
    void start_proxies(void);
    void stop_proxies(void);
    void handle_tunnel_message(const pfk::tcptunnel::TunnelMessage_m
                               &inbound);

    struct acceptorthread_args : public thread_args {

    };
    void acceptorthread(thread_args *args);
    void accept_proxy(int proxy_index);

    struct proxythread_args : public thread_args {
        int proxy_index;
        pxfe_tcp_stream_socket * newsock;
    };
    void proxythread(thread_args *args);
};
