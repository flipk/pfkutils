/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

void itsfs_print_func( int arg, char * format, ... );
void itsfs_addlog( char * format, ... );

struct treeinfo {
    int                 slave_tcp_fd;
    int                 treenumber;
    int                 treeid;
    Inode_remote_tree * inort;
    char *              treename;

    /**/ treeinfo   ( void );
    void init       ( void );
    void setup      ( char * newfsname, int newfd, int _treenumber, 
                      Inode_remote_tree * irt, int _treeid );
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
