/*
    This file is part of the "pfkutils" tools written by Phil Knaack
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

#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include "dll2.H"
#include "pk_msg_int.H"
#include <pthread.h>

class PK_Message_Queue_List;

class PK_Message_Manager {
    pthread_mutex_t   mutex;
    PK_Message_Queue_List * queues;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
public:
    PK_Message_Manager( void );
    ~PK_Message_Manager( void );
//
    int          create( char * name );  // returns qid
    int          lookup( char * name );  // returns qid or -1
    bool         destroy( int qid ); // returns false if qid does not exist
    bool         send( int qid, pk_msg_int * msg ); // returns false if error
    pk_msg_int * recv( int num_qids, int * qids,
                       int * retqid, int ticks );
};

extern PK_Message_Manager * PK_Messages_global;

#endif /* __MESSAGES_H__ */
