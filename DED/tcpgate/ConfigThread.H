
class ConfigThread : public Thread {
    static const int myprio = 4;
    static const int mystack = 65536;
    void entry( void );
    int fd;
    void vprintf( char * format, va_list ap );
    void printf( char * format, ... );
    void flush( void );
    bool getline( char * buf, int len );
    FILE * fdfw;
public:
    ConfigThread( int _fd );
    ~ConfigThread( void );
};

class ConfigFile {
    bool ok;
    static const char * config_file_name;
public:
    ConfigFile( void );
    ~ConfigFile( void );
    bool isok( void ) { return ok; }
    void create_threads( void );
};
