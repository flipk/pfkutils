
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

enum { PK_TIMER_OQ, PK_TIMER_HASH, PK_TIMER_NUMLISTS };

enum pk_timer_type { PK_TIMER_MSG, PK_TIMER_FUNC, PK_TIMER_COND };

class PK_Timer {
public:
    LListLinks <PK_Timer> links[PK_TIMER_NUMLISTS];
    int ordered_queue_key;  // initially, ticks
    int tid;
//    int set_time;  // only useful for debugging
    int expire_time;
    pk_timer_type type;
    union {
        struct { // type == PK_TIMER_MSG
            int qid;
            pk_msg_int * msg;
        } msg;
        struct { // type == PK_TIMER_FUNC
            void (*func)(void *);
            void * arg;
        } func;
        struct { // type == PK_TIMER_COND
            PK_Timeout_Obj * pkto;
        } cond;
    } u;
};

class PK_Timer_hash_1 {
public:
    static int hash_key( PK_Timer * item ) { return item->tid; }
    static int hash_key( int key ) { return key; }
    static bool hash_key_compare( PK_Timer * item, int key )
    { return ( item->tid == key ); }
};

class PK_Timer_List {
    LListOrderedQ <PK_Timer,       PK_TIMER_OQ  >  oq;
    LListHash     <PK_Timer,  int,
                   PK_Timer_hash_1,PK_TIMER_HASH>  hash;
public:
    PK_Timer_List( void ) { }
    void add( PK_Timer * pkt ) {
        oq.add( pkt, pkt->ordered_queue_key ); hash.add( pkt );
    }
    void remove( PK_Timer * pkt ) { oq.remove( pkt ); hash.remove( pkt ); }
    PK_Timer * get_head( void ) { return oq.get_head(); }
    PK_Timer * get_next( PK_Timer * pkt ) { return oq.get_next(pkt); }
    PK_Timer * find( int tid ) { return hash.find(tid); }
//    int get_cnt( void ) { return hash.get_cnt(); }
//    bool onlist( PK_Timer * pkt ) { return oq.onlist(pkt); }
//    bool onthislist( PK_Timer * pkt ) { return oq.onthislist(pkt); }
};
