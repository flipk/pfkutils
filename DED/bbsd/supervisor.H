
class Supervisor : public Thread {
    static const int MY_PRIO = 10;
    void entry( void );
public:
    class Messages * global_msgs;
    class Clients * clients;
    Supervisor( int max_messages );
    ~Supervisor( void );
    void enqueue( char * message, int size );
    static void status_function( void );
};

extern Supervisor * supervisor;
