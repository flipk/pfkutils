
#ifndef __TEST_H__
#define __TEST_H__

#include <netinet/in.h>

class app_thread : public PK_Thread {
    /* virtual */ void entry( void );
    int instance;
public:
    app_thread( int _instance );
    ~app_thread(void);
    int app_rx_qid; // the qid that I create to receive msgs from swrx
    int sw_tx_qid; // the qid that swtx creates to receive msgs from me
};

class net_thread : public PK_Thread {
    /* virtual */ void entry( void );
    struct sockaddr_in remote_sa;
    int fd;
    void register_my_fd(void);
    int pkfdid;
    void handle_net_rx(void);
public:
    net_thread( short local_port,
                char * remote_addr, short remote_port );
    ~net_thread(void);
//
    int net_sw_qid; // the qid that I create to receive msgs from sw
//
    int sw_tx_qid; // the qid that swtx created to receive msgs from me
    int sw_rx_qid; // the qid that swrx created to receive msgs from me
};

#endif /* __TEST_H__ */
