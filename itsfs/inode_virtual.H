
/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <dirent.h>
#include "inode.H"
#include "id_name_db.H"
#include "control_pipe.H"

class Inode_virtual;

class Inode_virtual_tree : public Inode_tree {
    friend class Inode_virtual;
    LList <Inode_virtual,0>  inos;
    uchar * (*update_status_info)( void );
    void (*command_handler)( char *, int );
    char * help_string;
public:
    Inode_virtual_tree            ( int _tree_id,
                                    uchar * (*update_status_info)( void ),
                                    void (*command_handler)( char *, int ),
                                    char * help_string );
    virtual ~Inode_virtual_tree   ( void );

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
    virtual bool      valid       ( void ) { return true; }

    // not in Inode_tree interface....

    int register_tree( uchar * name, Inode_tree * newtree );
    Inode_tree * kill_tree( int id );
};

class Inode_virtual : public Inode {
public:
    enum ITVT {
        INO_TREE_VIRT_ROOT_DIR,
        INO_TREE_VIRT_ROOT_PARENT_DIR,
        INO_TREE_VIRT_STATUS_FILE,
        INO_TREE_VIRT_CMD_FILE,
        INO_TREE_VIRT_HELP_FILE,
        INO_TREE_VIRT_TREE,
        INO_TREE_VIRT_CONTROL_COMM
    };

public:
    LListLinks<Inode_virtual>  links[1];
private:
    uchar * name;
    ITVT itype;
    Inode_virtual_tree * it;
    int dirposval;
    bool bad;
    Inode_tree * owned_tree;
    Control_Pipe * cpipe;

    uchar * status_info;
    void update_status_info( void );

    friend class Inode_virtual_tree;
public:
    Inode_virtual               ( Inode_virtual_tree * it,
                                  int file_id, ITVT itype );
    Inode_virtual               ( Inode_virtual_tree * it,
                                  int file_id,
                                  uchar * path );
    Inode_virtual               ( Inode_virtual_tree * it,
                                  int file_id, uchar * path,
                                  Inode_tree * owned_tree );
    virtual ~Inode_virtual      ( void );

    virtual int       isattr    ( sattr & );
    virtual int       ifattr    ( fattr & );

    virtual int       iread     ( int offset, uchar * buf, int &size );
    virtual int       iwrite    ( int offset, uchar * buf, int size );

    virtual int       setdirpos ( int cookie );
    virtual int       readdir   ( int &fileno, uchar * filename );

    virtual int       readlink    ( uchar * buf );
};
