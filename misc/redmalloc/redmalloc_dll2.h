
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

#ifndef __DLL2_H_
#define __DLL2_H_

// NOTE:  - LList::add adds at the 'tail' of the list.
//        - 'next' pointers point towards the tail, 'prev' towards head.
//        - LList::add_after adds to 'existing's' 'next' pointer so new
//             item is closer to tail.
//        - LList::add_before adds to 'existing's' 'prev' pointer so new
//             item is closer to head.

template <class T, int instance> class LList;

#ifndef DLL2BAIL
#define DLL2BAIL(reason) { printf reason ; kill( 0, 6 ); }
#endif

template <class T>
class LListLinks
{
private:
    friend class LList<T,0>;  // hack! hack! hack! --bill the cat
    friend class LList<T,1>;
    friend class LList<T,2>;
    friend class LList<T,3>;
    T * next;
    T * prev;
    void * onlist;
public:
    LListLinks( void ) {
        next = prev = NULL;
        onlist = 0;
    }
    ~LListLinks( void ) {
        if ( onlist != 0 )
            DLL2BAIL(( "ERROR LLIST ITEM DELETED BUT STILL ON A LIST!\n" ));
    }
};

// to use LList, type T must have the following member:
//    - LListLinks<T>  links[ DLL2_COUNT ];

template <class T, int instance>
class LList
{
    T * head;
    T * tail;
    int cnt;
public:
    LList( void ) { head = tail = NULL;  cnt = 0; }
    void add( T * x ) {
        LListLinks<T> * ll = & x->links[instance];
        if ( onlist( x )) 
            DLL2BAIL(( "ERROR LLIST ENTRY ALREADY ON LIST %#x\n",
                   (int)ll->onlist ));
        ll->onlist = (void*)this;
        ll->next = NULL;
        ll->prev = tail;
        if ( head )
            tail->links[instance].next = x;
        else
            head = x;
        tail = x;
        cnt++;
    }
    void remove( T * x ) {
        LListLinks<T> * ll = & x->links[instance];
        if ( !onthislist( x ))
            DLL2BAIL(( "ERROR LLIST ENTRY NOT ON THIS LIST %#x != %#x\n",
                   (int)ll->onlist, (int)this ));
        ll->onlist = 0;
        if ( ll->next )
            ll->next->links[instance].prev = ll->prev;
        else
            tail = ll->prev;
        if ( ll->prev )
            ll->prev->links[instance].next = ll->next;
        else
            head = ll->next;
        ll->next = ll->prev = NULL;
        cnt--;
    }
    void add_after( T * item, T * existing ) {
        LListLinks<T> * exll = & existing->links[instance];
        LListLinks<T> * itll = & item->links[instance];
        if ( !onthislist( existing ))
            DLL2BAIL(( "ERROR CANNOT DO add_after WHEN EXISTING ITEM "
                   "NOT ON THIS LIST\n" ));
        if ( onlist( item ))
            DLL2BAIL(( "ERROR CANNOT DO add_after WHEN NEW ITEM "
                   "IS ALREADY ON A LIST\n" ));
        itll->onlist = (void*)this;
        itll->next = exll->next;
        itll->prev = existing;
        exll->next = item;
        if ( itll->next )
            itll->next->links[instance].prev = item;
        else
            tail = item;
        cnt++;
    }
    void add_before( T * item, T * existing ) {
        LListLinks<T> * exll = & existing->links[instance];
        LListLinks<T> * itll = & item->links[instance];
        if ( !onthislist( existing ))
            DLL2BAIL(( "ERROR CANNOT DO add_after WHEN EXISTING ITEM "
                   "NOT ON THIS LIST\n" ));
        if ( onlist( item ))
            DLL2BAIL(( "ERROR CANNOT DO add_after WHEN NEW ITEM "
                   "IS ALREADY ON A LIST\n" ));
        itll->onlist = (void*)this;
        itll->prev = exll->prev;
        itll->next = existing;
        exll->prev = item;
        if ( itll->prev )
            itll->prev->links[instance].next = item;
        else
            head = item;
        cnt++;
    }
    bool onlist ( T * t ) {
        return t->links[instance].onlist != 0;
    }
    bool onthislist ( T * t ) {
        return t->links[instance].onlist == (void*)this; 
    }
    int get_cnt  ( void ) {
        return cnt;
    }
    T * dequeue_head( void ) {
        T * ret = head;
        if ( ret ) remove( ret );
        return ret;
    }
    T * dequeue_tail( void ) {
        T * ret = tail;
        if ( ret ) remove( ret );
        return ret;
    }
    T * get_head ( void ) {
        return head;
    }
    T * get_tail ( void ) {
        return tail;
    }
    T * get_next( T * x ) {
        if ( !onthislist( x ))
            DLL2BAIL(( "ERROR get_next ITEM is not on this list!\n" ));
        return x->links[instance].next;
    }
    T * get_prev( T * x ) {
        if ( !onthislist( x ))
            DLL2BAIL(( "ERROR get_prev ITEM is not on this list!\n" ));
        return x->links[instance].prev;
    }

    static void * operator new( ssize_t s   ) { return _real_malloc( s ); }
    static void   operator delete( void * p ) {        _real_free( p );   }
};

// to use LListHash, type T must have the following method, preferably inline:
//    - int hash_key(void)

template <class T,int instance>
class LListHash {
private:
    int hashsize;
    typedef LList <T,instance>  theLListHash;
    theLListHash * hash;
    int hashv ( int key ) { return key % hashsize; }
    int count;
public:
    LListHash ( int _hashsize ) {
        hashsize = _hashsize;
        hash = (theLListHash *)
            _real_malloc( sizeof( theLListHash ) * hashsize );
        memset( hash, 0, sizeof( theLListHash ) * hashsize );
        count = 0;
    }
    ~LListHash ( void ) {
        int i;
        T * t, * nt;
        for ( i = 0; i < hashsize; i++ )
            for ( nt = 0, t = hash[i].get_head(); t; t = nt )
            {
                nt = hash[i].get_next( t );
                hash[i].remove( t );
                // cannot delete
                // delete t;
            }
        _real_free( hash );
    }
    T * find    ( int key ) {
        int h = hashv( key );
        T * ret;
        for ( ret = hash[h].get_head(); ret; ret = hash[h].get_next(ret))
            if ( ret->hash_key() == key )
                break;
        return ret;
    }
    int get_cnt( void ) { return count; }
    void add( T * t ) { hash[ hashv( t->hash_key() ) ].add( t ); }
    void remove( T * t ) { hash[ hashv( t->hash_key() ) ].remove( t ); }
    bool onlist( T * t ) {
        return hash[ hashv( t->hash_key() ) ].onlist( t );
    }
    bool onthislist( T * t ) {
        return hash[ hashv( t->hash_key() ) ].onthislist( t );
    }
};

template <class T, int instance>
class LListLRU {
private:
    LList <T,instance> list;
public:
    int get_cnt( void ) { return list.get_cnt(); }
    void add( T * t ) { list.add( t ); }
    void remove( T * t ) { list.remove( t ); }
    void get_oldest( void ) { list.get_head(); }
    void promote( T * t ) { list.remove( t ); list.add( t ); }
    bool onlist( T * t ) { return list.onlist( t ); }
    bool onthislist( T * t ) { return list.onthislist( t ); }
    T * dequeue_head( void ) { 
        T * ret = list.get_head();
        if ( ret ) remove( ret );
        return ret;
    }
    T * dequeue_tail( void ) { 
        T * ret = list.get_tail();
        if ( ret ) remove( ret );
        return ret;
    }
    T * get_head( void ) { return list.get_head(); }
    T * get_tail( void ) { return list.get_tail(); }
    T * get_next( T * t ) { return list.get_next( t ); }
    T * get_prev( T * t ) { return list.get_prev( t ); }
};

template <class T, int hash_instance, int lru_instance>
class LListHashLRU {
private:
    LListHash <T, hash_instance> hash;
    LListLRU  <T, lru_instance > lru;
public:
    LListHashLRU ( int hashsize ) : hash( hashsize ) { }
    ~LListHashLRU( void ) { }
    T * find( int key ) { return hash.find( key ); }
    int get_cnt( void ) { return lru.get_cnt(); }
    void add( T * t ) { hash.add( t ); lru.add( t ); }
    void remove( T * t ) { hash.remove( t ); lru.remove( t ); }
    void promote( T * t ) { lru.promote( t ); }
    bool onlist( T * t ) { return lru.onlist( t ); }
    bool onthislist( T * t ) { return lru.onthislist( t ); }
    T * dequeue_lru_head( void ) {
        T * ret = lru.get_head();
        if ( ret ) remove( ret );
        return ret;
    }
    T * dequeue_lru_tail( void ) {
        T * ret = lru.get_tail();
        if ( ret ) remove( ret );
        return ret;
    }
    T * get_lru_head ( void ) { return lru.get_head(); }
    T * get_lru_tail ( void ) { return lru.get_tail(); }
    T * get_lru_next( T * t ) { return lru.get_next(t); }
    T * get_lru_prev( T * t ) { return lru.get_prev(t); }
};

// to use LListOrderedQ, type T must have the following member:
//    - int ordered_queue_key

template <class T, int instance>
class LListOrderedQ {
private:
    LList <T,instance> oq;
public:
    int get_cnt( void ) { return oq.get_cnt(); }
    void add( T * t, int new_oq_key ) {
        T * cur, * prev;
        for ( prev = 0, cur = oq.get_head();
              cur;
              prev = cur, cur = oq.get_next( cur ))
        {
            if ( new_oq_key < cur->ordered_queue_key )
            {
                cur->ordered_queue_key -= new_oq_key;
                break;
            }
            new_oq_key -= cur->ordered_queue_key;
        }
        t->ordered_queue_key = new_oq_key;
        if ( cur )
            oq.add_before( t, cur );
        else if ( prev )
            oq.add_after( t, prev );
        else
            oq.add( t );
    }
    void remove( T * t ) {
        T * n = oq.get_next( t );
        oq.remove( t );
        if ( n )
            n->ordered_queue_key += t->ordered_queue_key;
    }
    T * get_head( void ) { return oq.get_head(); }
    T * get_next( T * t ) { return oq.get_next( t ); }
    bool onlist( T * t ) { return oq.onlist( t ); }
    bool onthislist( T * t ) { return oq.onthislist( t ); }
};

#endif /* __DLL2_H_ */
