/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <dirent.h>
#include "inode.H"
#include "remote_ino.H"
#include "dll2.H"

class Inode_remote;

class Inode_remote_tree : public Inode_tree {
public:
    enum { DLL2_HASH, DLL2_LRU, NUM_DLL2 };
    struct  Inode_remote_tree_hash_1;
private:
    uchar * rootpath;

    remote_inode_client_tcp rict;

    static const int  MAX_HASH = 60;

    int zerorefs; // count of members whose refcount is zero
    bool bad_fd;

    LListHashLRU<Inode_remote,int,Inode_remote_tree_hash_1,
                 DLL2_HASH,DLL2_LRU> hashlru;

    friend class Inode_remote;

    Inode *     _create           ( int dirid,
                                    uchar * name, uchar * target,
                                    inode_file_type nftype, bool hardlink );
    bool        get_stats_somehow ( char * path, int fileid,
                                    struct stat * sb );
    Inode *     _get              ( bool hashonly, int file_id );
    Inode *     _get_parent       ( bool hashonly, int file_id );

public:
    /**/        Inode_remote_tree ( int tree_id,
                                    uchar * path, int fd );
    virtual    ~Inode_remote_tree ( void );

    virtual Inode *   get         ( int file_id )
        { return _get( false, file_id ); }
    virtual Inode *   get         ( int dir_id, uchar * filename );
    virtual Inode *   get_parent  ( int file_id )
        { return _get_parent( false, file_id ); }

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
    Inode_remote_tree * it;
    struct stat sb;
    int last_stat;
    bool refresh_directory;
    remino_readdir2_entry * dirlist;
    remino_readdir2_entry * curdir;
    void free_dirlist( void );

    friend class Inode_remote_tree;
public:
    LListLinks<Inode_remote> links[ Inode_remote_tree::NUM_DLL2 ];
    /**/          Inode_remote  ( Inode_remote_tree * it, uchar * path,
                                  int _fileid, inode_file_type );
    virtual      ~Inode_remote  ( void );

    virtual int       isattr    ( sattr & );
    virtual int       ifattr    ( fattr & );

    virtual int       iread     ( int offset, uchar * buf, int &size );
    virtual int       iwrite    ( int offset, uchar * buf, int size );

    virtual int       setdirpos ( int cookie );
    virtual int       readdir   ( int &fileno, uchar * filename );

    virtual int       readlink  ( uchar * p );
};
