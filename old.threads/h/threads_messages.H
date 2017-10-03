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

#include <string.h>

class MessageAddress {
public:
    UINT32_t eid;  // entity identifier
    UINT32_t mid;  // message queue identifier
    MessageAddress( void )
        : eid( 0 ), mid( 0 ) { }
    MessageAddress( UINT32 _mid )
        : eid( 0 ), mid( _mid ) { }
    MessageAddress( UINT32 _eid, UINT32 _mid )
        : eid( _eid ), mid( _mid ) { }
    void set( UINT32 _mid ) {
        mid.set( _mid );
    }
    void set( UINT32 _eid, UINT _mid ) { 
        eid.set( _eid );
        mid.set( _mid );
    }
};

class Message {
public:
    static const unsigned int
    MAGIC = MagicNumbers_MessageEncoding_Header;

private:
    static const unsigned int
    ENCMAGIC = MagicNumbers_MessageEncoding_HeaderEnc;

    //xxx
    static const int ENCRYPT_SKIP = sizeof( UINT32_t ) * 2;
    static const int THIS_SKIP = sizeof( LListLinks<Message> );

protected:
    // derived class should call this constructor
    // with _size equal to sizeof( whole class )
    Message( int _size, int _type )
        : magic( MAGIC ), size( _size - THIS_SKIP ),
          encrypt_magic( ENCMAGIC ),
          random1( random() ),
          random2( random() ),
          type( _type )
        { }

public:
    LListLinks<Message> links[1];
    // everything after this should be moved about
    // by alternate transports. get_body returns this.
    // these first two fields are not encrypted.
    UINT32_t magic;  // MAGIC
    UINT32_t size;   // size from 'magic' to end of body

private:
    // everything after this is encrypted.
    UINT32_t encrypt_magic;
    UINT32_t random1;
    UINT32_t random2;

public:
    UINT32_t type;
    UINT32_t source_tid;
    MessageAddress src;
    MessageAddress dest;

    UCHAR * get_body( void ) {
        return ((UCHAR*)this)+THIS_SKIP;
    }

    int get_size( void ) { return size.get(); }
    bool valid_magic( void ) {
        return ( magic.get() == MAGIC );
    }

    bool is_encrypted( void ) {
        return ( encrypt_magic.get() != ENCMAGIC );
    }

    bool encrypt( encrypt_iface * crypt ) {
        random1.set( random() );
        random2.set( random() );
        int _size = size.get() - ENCRYPT_SKIP;
        UCHAR * temp_buf = new UCHAR[ _size ];
        bool encr_ret =
            crypt->encrypt( temp_buf, get_body() + ENCRYPT_SKIP, _size );
        if ( encr_ret == true )
            memcpy( get_body() + ENCRYPT_SKIP, temp_buf, _size );
        delete[] temp_buf;
        return encr_ret;
    }

    bool decrypt( encrypt_iface * crypt ) {
        UINT32 _size = size.get() - ENCRYPT_SKIP;
        UCHAR * temp_buf = new UCHAR[ _size ];
        bool encr_ret = 
            crypt->decrypt( temp_buf,
                            get_body() + ENCRYPT_SKIP,
                            _size );
        if ( encr_ret == true )
        {
            memcpy( get_body() + ENCRYPT_SKIP, temp_buf, _size );
            delete[] temp_buf;
            if ( encrypt_magic.get() != ENCMAGIC )
                return false;
            return true;
        }
        else
        {
            delete[] temp_buf;
            TH_DEBUG( DEBUG_MSGS,
                      ( 0, "decrypt", "crypt->decrypt failed\n" ));
            return false;
        }
    }
};

#define MsgDef( classname, typevalue, body ) \
class classname : public Message { \
public: \
    static const unsigned int TYPE = typevalue; \
    classname( void ) : Message( sizeof( classname ), TYPE ) { } \
    body \
}

#define MsgDef2( classname, typevalue, constructorargs, constructor, body ) \
class classname : public Message { \
public: \
    static const unsigned int TYPE = typevalue; \
    classname( constructorargs ) : Message( sizeof( classname ), TYPE ) \
        { constructor } \
    body \
}

MsgDef( MessageGeneric, MagicNumbers_MessageEncoding_GenericType,
        static const int DATA_SIZE = 16384;
        UCHAR data[ DATA_SIZE ];
    );

struct messageQueue;

class ThreadMessages {
public:
    enum fd_mq_t {
        FOR_NOTHING   = 0,
        FOR_READ      = 1,
        FOR_WRITE     = 2,
        FOR_READWRITE = 3, // = FOR_READ + FOR_WRITE, intentional
        FOR_ERROR     = 4  // only in indication response
    };
    ThreadMessages( int max_mqids, int my_eid,
                    int max_eids, int max_fds );
    ~ThreadMessages( void );

private:
    int max_mqids;
    int my_eid;
    int max_eids;
    int max_fds;
    messageQueue ** mqs;
    int * eid_mqids; // for routing to other eids
    struct fd_mqids_t {
        fd_mqids_t( void ) { mqid = -1; type = FOR_NOTHING; arg = NULL; }
        int mqid;
        fd_mq_t type;
        void * arg;
    } * fd_mqids;
    bool die;
    Threads::tid_t fd_helper_tid;
    Threads::tid_t lookup_helper_tid;
    static void msgs_fd( void * arg );
    void _msgs_fd( void );
    static void lookup_thd( void * arg );
    void _lookup_thd( void );
    void send_indications( int num, int * outs );
    void send_error_indication( int fd );
    void _unregister_fd_mq( int fd );
    void kill( void ) {
        die = true;
        th->resume( fd_helper_tid );
        th->resume( lookup_helper_tid );
    }
    static const char * lookup_mqname;
    friend class Threads;

public:
    void printmqs( FILE * f );

    int get_my_eid( void ) { return my_eid; }
    int get_max_eids( void ) { return max_eids; }

    bool register_mq( int &mqid, char * mqname );
    bool unregister_mq( int mqid );

    bool lookup_mq( int eid, int &mqid, char * mqname );
    char * get_mqname( int mqid );

    bool send( Message *, MessageAddress * );
    Message * recv( int numqids, int * mqids,
                    int * mqid_out, int timeout );

    bool register_eid( int eid, int mqid );
    bool unregister_eid( int eid );

    bool register_fd_mq( int fd, void * arg, fd_mq_t, int mqid );
    bool unregister_fd_mq( int fd );
};

MsgDef( FdActiveMessage, MagicNumbers_Messages_MessageFdActive,
        int fd;
        void * arg;
        int activity;  // this is a ThreadMessages::fd_mq_t
    );
