
class Client : public Thread {
    static const int MY_PRIO = 8;

    // filled from the line
    static const int bufsize = 256;
    char _inbuf[ bufsize ];
    int  _inbufpos;
    int  _inbuflen;
    int  _inbufleft;

    // get char from line (or buf), returns -1 if error on line
    int  getc( bool return_telnet );
    int _getc( void );

    static const int linelen = 80;
    char inbuf  [ linelen+2 ];
    char inbuf2 [ linelen+2 ];
    int  inbuflen;

    static const int maxargs = 12;
    char * args   [ maxargs ];
    int    arglen [ maxargs ];
    int    numargs;

    // calls getc to fill inbuf, then copies to inbuf2, and 
    // splits inbuf2 into args.
    bool getline( void );

    // return line from arg n to end as single line
    char * line_remainder( int argnum ) {
        return inbuf + ( args[argnum] - inbuf2 );
    }

    Client * next;
    int fd;
    unsigned long addr;
    class MessageIterator * iter;

    void entry            ( void );
    bool handle_input     ( void );
    bool negotiate_telnet ( void );
    bool negotiate_user   ( void );

    // send data only to the connected user
    void print   ( char * format, ... );

    // broadcast data to all other users
    void print_b ( char * format, ... );

    bool command_quit     ( void );
    bool command_emote    ( void );
    bool command_b        ( void );
    bool command_who      ( void );
    bool command_kill     ( void );
    bool command_nick     ( void );

public:
    static const int maxname = 30;
    char username[ maxname+1 ];

    Client( int _fd, unsigned long _addr );
    ~Client( void );

    friend class Clients;
};
