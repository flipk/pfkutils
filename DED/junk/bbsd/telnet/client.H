
class Client : public Thread {
    enum constants {
        MY_PRIO    = 8,
        linelen    = 120,
        log_user   = 1,
        log_server = 2
    };
    int fd_user;
    int fd_server;
    void entry( void );
    void print( char * format, ... );
    void log( int which, char * buf, int len );
public:
    Client( int fd, unsigned int addr );
    ~Client( void );
};

