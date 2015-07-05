
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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
