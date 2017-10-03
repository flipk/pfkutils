
class PrintMessage : public Message {
public:
    static const unsigned int
    TYPE = MagicNumbers_Threads_PrintMessage;
    static const int MAX_MSG = 1024;
    UINT32_t len;
    char printbody[1];
    static PrintMessage * newPrintMessage( int len ) {
        return new(len) PrintMessage(len);
    }
private:
    PrintMessage( int size )
        : Message( sizeof( PrintMessage ) + size, TYPE ),
          len( size ) { }
    void * operator new( size_t s, int len ) {
        char * ret = new char[ s + len ];
        return (void*)ret;
    }
};

class ThreadPrinter : public Thread {
    void entry( void );
    int mqid;
    bool die;
public:
    ThreadPrinter( void )
        : Thread( "printer", 1, 16384 ) {
        mqid = -1;
        resume( tid );
    }
    void kill( void ) { die = true; resume( tid ); }
    friend class Threads;
};
