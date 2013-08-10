
class DataThread : public Thread {
    static const int myprio = 8;
    static const int mystack = 65536;
    void entry( void );
    int port, remotehost, remoteport;
    int connfromhost, connfromport;
    int fds[2];  //0 is initiator, 1 is target
    int logfds[2]; //0 is log of initiator, 1 is log of target
public:
    DataThread( int fd,
                int connfromhost, int connfromport,
                int remotehost, int remoteport );
    ~DataThread( void );
};
