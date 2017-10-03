/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

void itsfs_print_func( int arg, char * format, ... );
void itsfs_addlog( char * format, ... );

struct treeinfo {
    int           slave_tcp_fd;
    int           treenumber;
    int           treeid;
    Inode_tree *  inot;
    char *        treename;

    /**/ treeinfo   ( void );
    void init       ( void );
    void setup      ( char * newfsname, int newfd, int _treenumber, 
                      Inode_tree * it, int _treeid );
    void teardown   ( void );
    bool valid      ( void );
    void sprintinfo ( char * f );
    void printcache ( void );
};

class svr_globals {
    static const int maxvs = 20;
public:
    nfssrv *             server;
    Inode_virtual_tree * itv;
    int                  nfs_rpc_udp_fd;
    int                  slave_rendevous_fd;
    treeinfo             trees[ maxvs ];
    int                  source_port;
    int                  nfs_rpc_call_count;
    int                  exit_command;
    char *               close_command;
    struct timeval       starttime;

    /**/ svr_globals   ( nfssrv * _server, Inode_virtual_tree * _itv );
    /**/ ~svr_globals  ( void );
    void set_close     ( char * cmd, int len );
    void print_cache   ( void );
    void sprintinfo    ( char * f );
    int  new_index     ( void );
    void kill_tree     ( int i );
    int  fdset         ( fd_set * rfds );
    void checkfds      ( fd_set * rfds );
    void check_bad_fds ( void );
    int  findname      ( char * f );
};
