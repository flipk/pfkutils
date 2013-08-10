
#undef CLIENTS_DEBUG

class Client;

class Clients {
    Client * first;
    Client * last;
    int num_clients;
    ThreadSemaphore * sem;
    void lock(void) { global_th->take( sem, Threads::WAIT_FOREVER ); }
    void unlock(void) { global_th->give( sem ); }
#ifdef CLIENTS_DEBUG
    void dump( void );
#endif
public:
    Clients( void );
    ~Clients( void );
    void add_client( Client * );
    void del_client( Client * );
    char ** get_usernames( void );
    void status_function( void );
    int get_num_clients( void ) { return num_clients; }
};
