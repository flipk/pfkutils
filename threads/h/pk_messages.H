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
#include <pthread.h>

#include "types.H"

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

#define PkMsgIntDef( classname, typevalue, body ) \
class classname : public pk_msg_int { \
public: \
    static const UINT16 TYPE = typevalue ; \
    classname( void ) : pk_msg_int( sizeof( classname ), TYPE ) { } \
    body \
}

// the COMMA definition is useful if the 'constructor' argument below
// would like to insert member-constructions.  yeah, i know, it's kind
// of gross -- i admit, not my finest hour.
#define COMMA ,

#define PkMsgIntDef2( classname, typevalue, body, \
                      constructargs, constructor ) \
class classname : public pk_msg_int { \
public: \
    static const UINT16 TYPE = typevalue ; \
    classname( void ) : pk_msg_int( sizeof( classname ), TYPE ) { } \
    classname constructargs : pk_msg_int( sizeof( classname ), TYPE ) \
        constructor \
    body \
}

struct pk_msg_int {
    LListLinks <pk_msg_int> links[1];
    UINT16    length; // automatically filled by PkMsgIntDef*
    UINT16    type; // automatically filled by PkMsgIntDef*
    UINT32    src_q; // user must fill
    UINT32    dest_q; // user must fill
//
    pk_msg_int( UINT16 _length, UINT16 _type )
        : length( _length ), type( _type ) { }
    template <class T> bool convert( T ** ptr ) {
        if ( type != T::TYPE )
            return false;
        *ptr = (T*)this;
        return true;
    }
};

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
