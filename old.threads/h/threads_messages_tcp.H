/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <netinet/in.h>
#include <netdb.h>

class MsgsTcpInd : public Message {
public:
    static const unsigned int
    TYPE = MagicNumbers_Messages_MsgsTcpIndication;
    MsgsTcpInd( void ) 
        : Message( sizeof( MsgsTcpInd ), TYPE ) { }

    enum ind_type {
        IND_STARTED          = 0,
        IND_CONN_ESTABLISHED = 1,
        IND_CONN_HAVE_EID    = 2,
        IND_CONN_FAILED      = 3,
        IND_CONN_CLOSED      = 4,
        NUM_IND_TYPES        = 5
    };
    UINT32_t connid;
    UINT32_t ind;       // this is an ind_type
    UINT32_t other_eid;
    // the fields within this array should match
    // the enum defs above.
    static const char * ind_names[NUM_IND_TYPES];
};

class MessagesTcp : public Thread {
    int connid;
    unsigned long remote_ip;
    int remote_port;
    int fd;
    int fd_out;
    int other_eid;
    int other_req_mid;
    struct sockaddr_in sa;
    encrypt_iface * cr;
    encrypt_iface * new_cr;
    MessageAddress ind_dest;

    bool waiting_for_newkeyack;
    MessageAddress keyreq_sender;

    void entry( void );
    // in this attribute modifier, isn't it strange
    // how 'format' is argument 3, not 2? I think its
    // because in gcc a C++ method is implemented with
    // an implicit first argument (the object pointer).
    // just a theory...
    void print( int myerrno, char * format, ... )
        __attribute__ ((format (printf, 3, 4)));
    void send_indication( MsgsTcpInd::ind_type );
    void send_my_eid( void );
    bool forward_data( Message * );
    bool handle_fd_data( void );
    bool setup_fd_active_mq( void );

// - message format:
//
// aa aa aa aa bb bb bb bb cc cc cc cc
// <-connest-> <--myeid--> <-reqmid-->
//
//   aa aa aa aa bb bb bb bb cc cc cc cc .....
//   <msgprefix> <-msglen--> <---message----->
//
// states:
//  - headerhunt, 0 to 3
//  - eidcollect, 0 to 3
//  - msglencollect, 0 to 3
//  - messagecollect, 0 to n

    static const int BUFSIZE = 16384 + 8;
    char buffer1[ BUFSIZE ];
    char buffer2[ BUFSIZE ];
    int buf1_size;

    char header_connest[4];
    char header_msgprefix[4];

    enum _rcvrstate {
        HEADERHUNT, EIDCOLLECT, MSGLENCOLLECT, MESSAGECOLLECT
    } rcvrstate;
    int rcvrsubstate;
    int rcvrmsglen;

    void handle_newkey_reqint( Message * );
    void handle_newkey_ackint( Message * );
    void rcvr_statemachine( void );
    bool decryptmsg( Message * m );

    enum mbidnames {
        // order of this list defines priority of mailboxes,
        // higest to lowest
        REQ_EID = 0, OTHER_EID = 1, FD_ACTIVE = 2, NUM_MBIDS = 3
    };
    int mbids[ NUM_MBIDS ];

    MsgsTcpInd::ind_type exit_ind;

    static const int MY_PRIO = NUM_PRIOS-1;
    static const int MY_STACK = 16384;

    bool verbose;

public:
    MessagesTcp( int indication_mq, int connid, encrypt_iface * _cr,
                 unsigned long remote_ip, int remote_port,
                 bool _verbose );
    ~MessagesTcp( void );

    // kill the link
    void kill( void );

    // switch encryption keys on the link
    bool switch_key( encrypt_iface * newkey );
};
