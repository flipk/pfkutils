
#ifndef __MPROC_H_
#define __MPROC_H_

#include "threads.H"
#include "hsc.H"

// note that mproc tasks are created suspended and
// are not resumed until the hsc is created.

class ETHER_DISC_STATE_MACHINE;

class MPROC_Thread : public Thread {
    friend class ETHER_DISC_STATE_MACHINE;
    void entry( void );
    enum { MQ_HSC, MQ_ETHER, MQ_TIMER, MQ_N };
    int mqids[MQ_N];
    int mac;
    int cage;
    int slot;
    bool haslink;
    enum { CONTINUE, DIE,
           RESTART1, RESTART2 } whattodo;
    hsc * h;
    MPROC_Thread * other;
    char * name;
    ETHER_DISC_STATE_MACHINE * sm;
    // the following are called by the ether_disc state machine helper
    void iml_print( char * format, ... );
    int set_ether_timer( int ticks, int seq );
    void restart_audit( void );
    static void take_bcast( void * arg, hsc::hsc_take_bcast_type type );
public:
    MPROC_Thread( int _cage, int _slot, char * _name );
    ~MPROC_Thread( void );
    int get_hsc_mqid( void ) { return mqids[MQ_HSC]; }
    void wakeup( MPROC_Thread * _other, hsc * _h )
        { other = _other; h = _h; resume( tid ); }
};

#endif /* __MPROC_H_ */
