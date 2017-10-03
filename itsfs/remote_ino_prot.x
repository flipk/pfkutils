/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

%#define MAX_REQ 9000
%#define MAX_PATH 500

%#include "xdr.h"

typedef string filepath<MAX_PATH>;

enum remino_callnum {
    OPEN      =  1,
    CLOSE     =  2,
    READ      =  3,
    WRITE     =  4,
    TRUNCATE  =  6,
    UNLINK    =  7,
    OPENDIR   =  8,
    CLOSEDIR  =  9,
    READDIR   = 10,
    REWINDDIR = 11,
    MKDIR     = 12,
    RMDIR     = 13,
    LSTAT     = 14,
    RENAME    = 15,
    CHOWN     = 16,
    CHMOD     = 17,
    UTIMES    = 18,
    READLINK  = 19,
    LINK_H    = 20,
    LINK_S    = 21,
    READDIR2  = 22
};

struct remino_open_args {
    filepath path;
    int flags;
    int mode;
};

struct remino_close_args {
    int fd;
};

struct remino_read_args {
    int fd;
    int pos;
    int len;
};

struct remino_write_args {
    int fd;
    int pos;
    opaque data<MAX_REQ>;
};

struct remino_truncate_args {
    filepath path;
    int length;
};

/* used for unlink, opendir, rmdir, stat, readlink */
struct remino_path_args {
    filepath path;
};

/* used for closedir, readdir, rewinddir */
struct remino_dirptr_args {
    unsigned dirptr;
};

/* used for readdir2 */
struct remino_dirptr2_args {
    unsigned dirptr;
    int pos;
};

/* used for mkdir and chmod */
struct remino_path_mode_args {
    filepath path;
    int mode;
};

/* also used for hard link and soft link */
struct remino_rename_args {
    filepath from;   /* target  */
    filepath to;     /*  path   */
};

struct remino_chown_args {
    filepath path;
    int owner;
    int group;
};

struct remino_utimes_args {
    filepath path;
    unsigned asecs;
    unsigned ausecs;
    unsigned msecs;
    unsigned musecs;
};

union remino_call switch (remino_callnum call) {
 case OPEN:
     remino_open_args open;
 case CLOSE:
     remino_close_args close;
 case READ:
     remino_read_args read;
 case WRITE:
     remino_write_args write;
 case TRUNCATE:
     remino_truncate_args truncate;
 case UNLINK:
 case OPENDIR:
 case RMDIR:
 case LSTAT:
 case READLINK:
     remino_path_args path_only;
 case CLOSEDIR:
 case READDIR:
 case REWINDDIR:
     remino_dirptr_args dirptr_only;
 case READDIR2:
     remino_dirptr2_args dirptr2;
 case MKDIR:
 case CHMOD:
     remino_path_mode_args path_mode;
 case RENAME:
 case LINK_S:
 case LINK_H:
     remino_rename_args rename;
 case CHOWN:
     remino_chown_args chown;
 case UTIMES:
     remino_utimes_args utimes;
 default:
     void;
};

struct remino_open_reply {
    int err;
    int fd;
};

struct remino_errno_reply {
    int err;
    int retval;
};

struct remino_read_reply {
    int err;
    opaque data<MAX_REQ>;
};

struct remino_write_reply {
    int err;
    int size;
};

struct remino_opendir_reply {
    int err;
    unsigned dirptr;
};

struct remino_readdir_entry {
    int ftype;  /* actually inode_file_type */
    filepath name;
    int fileid;
};

struct remino_readdir_reply {
    int err;
    int retval;
    remino_readdir_entry entry;
};

struct remino_stat_reply {
    int err;
    int retval;
    int mode;
    int nlink;
    int uid;
    int gid;
    int size;
    int blocksize;
    int rdev;
    int blocks;
    int fsid;
    int asec;
    int ausec;
    int msec;
    int musec;
    int csec;
    int cusec;
    int fileid;
};

struct remino_readdir2_entry {
    remino_readdir_entry    dirent;
    remino_stat_reply       stat;
    remino_readdir2_entry * next;
};

struct remino_readdir2_reply {
    int err;
    int retval;
    remino_readdir2_entry * list;
    bool eof;
};

struct remino_readlink_reply {
    int err;
    int retval;
    filepath path;
};

union remino_reply switch (remino_callnum reply) {
 case OPEN:
     remino_open_reply open;
 case CLOSE:
 case TRUNCATE:
 case UNLINK:
 case CLOSEDIR:
 case REWINDDIR:
 case MKDIR:
 case RMDIR:
 case RENAME:
 case CHOWN:
 case CHMOD:
 case UTIMES:
 case LINK_H:
 case LINK_S:
     remino_errno_reply err;
 case READ:
     remino_read_reply read;
 case WRITE:
     remino_write_reply write;
 case OPENDIR:
     remino_opendir_reply opendir;
 case READDIR:
     remino_readdir_reply readdir;
 case READDIR2:
     remino_readdir2_reply readdir2;
 case LSTAT:
     remino_stat_reply stat;
 case READLINK:
     remino_readlink_reply readlink;
 default:
     void;
};
