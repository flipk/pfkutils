
class Acceptor : public Thread {
    static const int MY_PRIO = 9;
    void entry( void );
    int port;
    int fd;
public:
    Acceptor( int _port );
    ~Acceptor( void );
};
