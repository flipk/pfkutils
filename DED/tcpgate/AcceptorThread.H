
class AcceptorThread : public Thread {
    static const int myprio = 10;
    static const int mystack = 32768;
    void entry( void );
    int port, remotehost, remoteport;
    int fd;
public:
    enum {
        CONFIG_HOST,
        STATUS_HOST
    };
    AcceptorThread( int port, int remotehost, int remoteport );
    ~AcceptorThread( void );
};
