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

#include <dirent.h>
#include "inode.H"
#include "remote_ino.H"
#include "dll2.H"

class Inode_remote;

class Inode_remote_tree : public Inode_tree {
public:
    enum { DLL2_HASH, DLL2_LRU, NUM_DLL2 };
private:
    uchar * rootpath;

    remote_inode_client_tcp rict;

    static const int  MAX_HASH = 500;
    static const int HASH_SIZE = 100;

    int zerorefs; // count of members whose refcount is zero
    bool bad_fd;

    LListHashLRU<Inode_remote,DLL2_HASH,DLL2_LRU> hashlru;

    friend class Inode_remote;

    Inode         *   _create     ( int dirid,
                                    uchar * name, uchar * target,
                                    inode_file_type nftype, bool hardlink );

public:
    Inode_remote_tree             ( int tree_id,
                                    uchar * path, int fd );
    virtual ~Inode_remote_tree    ( void );

    virtual Inode *   get         ( int file_id );
    virtual Inode *   get         ( int dir_id, uchar * filename );
    virtual Inode *   get_parent  ( int file_id );
    virtual int       deref       ( Inode * );

    virtual Inode *   create      ( int dirid, uchar * name, inode_file_type );
    virtual Inode *   createslink ( int dirid,
                                    uchar * name, uchar * target );
    virtual Inode *   createhlink ( int dirid,
                                    uchar * name, int fileid );
    virtual int       destroy     ( int fileid, inode_file_type );
    virtual int       rename      ( int olddir, uchar * oldfile,
                                    int newdir, uchar * newfile );

    virtual void      clean       ( void );
    virtual void      print_cache ( int arg, void (*printfunc)
                                    (int arg, char *format,...) );
    virtual bool      valid       ( void ) { return !bad_fd; }
};

class Inode_remote : public Inode {
    uchar * path;
    int fd;
    DIR * dir;
    int dirpos;
    Inode_remote_tree * it;
    struct stat sb;
    int last_stat;

    friend class Inode_remote_tree;
public:
    LListLinks<Inode_remote> links[ Inode_remote_tree::NUM_DLL2 ];
    int hash_key( void ) { return file_id; }
    Inode_remote                ( Inode_remote_tree * it, uchar * path,
                                  int _fileid, inode_file_type );
    virtual ~Inode_remote       ( void );

    virtual int       isattr    ( sattr & );
    virtual int       ifattr    ( fattr & );

    virtual int       iread     ( int offset, uchar * buf, int &size );
    virtual int       iwrite    ( int offset, uchar * buf, int size );

    virtual int       setdirpos ( int cookie );
    virtual int       readdir   ( int &fileno, uchar * filename );

    virtual int       readlink  ( uchar * p );
};
