
#ifndef __NET_H__
#define __NET_H__

#include "pk_threads.H"
#include "messages.H"
#include "net_int.H"

class net_tx_thread : public PK_Thread {
    /*virtual*/ void entry( void );
    int my_data_qid; // from sw tx
    int my_cm_qid; // from cm thread
    int my_user_qid; // from user
    int user_qid; // to user
    int cm_qid; // to cm thread
    int net_fd; // to network
public:
    net_tx_thread(void);
    ~net_tx_thread(void);
    void get_my_qids(int *data_qid, int *cm_qid, int *user_qid) {
        *data_qid = my_data_qid;
        *cm_qid = my_cm_qid;
        *user_qid = my_user_qid;
    }
    void register_user_qid( int qid ) { user_qid = qid; }
    void register_net_fd( int fd ) { net_fd = fd; }
    void register_cm_qid( int qid ) { cm_qid = qid; }
};

class net_rx_thread : public PK_Thread {
    /*virtual*/ void entry( void );
    int my_qid;
    int net_fd;
public:
    net_rx_thread(void);
    ~net_rx_thread(void);
    int get_my_qid(void) { return my_qid; }
    void register_net_fd( int fd ) { net_fd = fd; }
};

#endif /* __NET_H__ */
