/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

/** \cond INTERNAL */

enum { PK_TIMER_OQ, PK_TIMER_HASH, PK_TIMER_NUMLISTS };

enum pk_timer_type { PK_TIMER_MSG, PK_TIMER_FUNC, PK_TIMER_COND };

class PK_Timer {
public:
    LListLinks <PK_Timer> links[PK_TIMER_NUMLISTS];
    int ordered_queue_key;  // initially, ticks
    int tid;
//    int set_time;  // only useful for debugging
    uint64_t expire_time;
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
    static int hash_key( const int key ) { return key; }
    static bool hash_key_compare( PK_Timer * item, const int key )
    { return ( item->tid == key ); }
};

class PK_Timer_List {
    LListOrderedQ <PK_Timer,       PK_TIMER_OQ  >  oq;
    LListHash     <PK_Timer,  int,
                   PK_Timer_hash_1,PK_TIMER_HASH>  hash;
public:
    PK_Timer_List( void ) { }
    void add( PK_Timer * pkt ) { oq.add( pkt, pkt->ordered_queue_key ); hash.add( pkt ); }
    void remove( PK_Timer * pkt ) { oq.remove( pkt ); hash.remove( pkt ); }
    PK_Timer * get_head( void ) { return oq.get_head(); }
    PK_Timer * get_next( PK_Timer * pkt ) { return oq.get_next(pkt); }
    PK_Timer * find( int tid ) { return hash.find(tid); }
//    int get_cnt( void ) { return hash.get_cnt(); }
//    bool onlist( PK_Timer * pkt ) { return oq.onlist(pkt); }
//    bool onthislist( PK_Timer * pkt ) { return oq.onthislist(pkt); }
};
/** \endcond */
