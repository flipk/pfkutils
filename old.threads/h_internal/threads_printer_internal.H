
MsgDef2( PrintMessage, MagicNumbers_Threads_PrintMessage
         ,
         int size
         ,
         len = size;
         ,
         static const int MAX_MSG = 1024;
         int len;
         char printbody[1];
         static PrintMessage * newPrintMessage( int len ) {
             return new(len) PrintMessage(len);
         }
         private:
         void * operator new( size_t s, int len ) {
             char * ret = new char[ s + len ];
             return (void*)ret;
         }
    );

class ThreadPrinter : public Thread {
    void entry( void );
    int mqid;
    bool die;
public:
    ThreadPrinter( void )
        : Thread( "printer", 1, 65536 ) {
        mqid = -1;
        resume( tid );
    }
    void kill( void ) { die = true; resume( tid ); }
    friend class Threads;
};
