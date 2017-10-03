
#ifndef __DLL2_H_
#define __DLL2_H_

/* configurable items */

#ifndef DLL2_INCLUDE_LOGNEW
#define DLL2_INCLUDE_LOGNEW 0
#endif

#ifndef DLL2_CHECKSUMS
#define DLL2_CHECKSUMS      1
#endif

#ifndef DLL2_INCLUDE_BTREE
#define DLL2_INCLUDE_BTREE  1
#endif

#ifndef DLL2BAIL
#include <stdio.h>
#define DLL2BAIL(reason) { printf reason ; kill( 0, 6 ); }
#endif

/* config items end here */

#include <sys/types.h>
#include <signal.h>
#if DLL2_INCLUDE_LOGNEW
#include "lognew.H"
#else
#define LOGNEW new
#endif

// NOTE:  - LList::add adds at the 'tail' of the list.
//        - 'next' pointers point towards the tail, 'prev' towards head.
//        - LList::add_after adds to 'existing's' 'next' pointer so new
//             item is closer to tail.
//        - LList::add_before adds to 'existing's' 'prev' pointer so new
//             item is closer to head.

template <class T, int instance> class LList;

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
#if DLL2_CHECKSUMS
    unsigned int checksum;
    unsigned int calc_checksum(void) {
        return 0xb09d3086U + (unsigned int) next +
            (unsigned int) prev + (unsigned int) onlist;
    }
    void validate(void) {
        if ( checksum != calc_checksum())
            DLL2BAIL(( "CHECKSUM ERROR IN DLL2 ELEMENT\n" ));
    }
    void recalc(void) { checksum = calc_checksum(); }
#else
    void validate(void) {}
    void recalc(void) {}
#endif
public:
    LListLinks( void ) {
        next = prev = NULL;
        onlist = 0;
        recalc();
    }
    ~LListLinks( void ) {
        if ( onlist != 0 )
            DLL2BAIL(( "ERROR LLIST ITEM DELETED BUT STILL ON A LIST!\n" ));
#if DLL2_CHECKSUMS
        checksum = 0;
#endif
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
#if DLL2_CHECKSUMS
    unsigned int checksum;
    unsigned int calc_checksum(void) {
        return 0x2df692f8U + (unsigned int) head +
            (unsigned int) tail + (unsigned int) cnt;
    }
    void validate(void) {
        if ( checksum != calc_checksum())
            DLL2BAIL(( "CHECKSUM ERROR IN DLL2 LLIST\n" ));
    }
    void recalc(void) { checksum = calc_checksum(); }
#else
    void validate(void) { }
    void recalc(void) { }
#endif
public:
    LList( void ) { head = tail = NULL;  cnt = 0; recalc(); }
    ~LList( void ) {
        validate();
        if ( head || tail || cnt )
            DLL2BAIL(( "LLIST DELETED BUT NOT EMPTY\n" ));
#if DLL2_CHECKSUMS
        checksum = 0;
#endif
    }
    void add( T * x ) {
        validate();
        LListLinks<T> * ll = & x->links[instance];
        ll->validate();
        if ( onlist( x )) 
            DLL2BAIL(( "ERROR LLIST ENTRY ALREADY ON LIST %#x\n",
                   (int)ll->onlist ));
        ll->onlist = (void*)this;
        ll->next = NULL;
        ll->prev = tail;
        if ( tail )
        {
            tail->links[instance].next = x;
            tail->links[instance].recalc();
        }
        else
            head = x;
        ll->recalc();
        tail = x;
        cnt++;
        recalc();
    }
    void remove( T * x ) {
        validate();
        LListLinks<T> * ll = & x->links[instance];
        ll->validate();
        if ( !onthislist( x ))
            DLL2BAIL(( "ERROR LLIST ENTRY NOT ON THIS LIST %#x != %#x\n",
                   (int)ll->onlist, (int)this ));
        ll->onlist = 0;
        if ( ll->next )
        {
            ll->next->links[instance].prev = ll->prev;
            ll->next->links[instance].recalc();
        }
        else
            tail = ll->prev;
        if ( ll->prev )
        {
            ll->prev->links[instance].next = ll->next;
            ll->prev->links[instance].recalc();
        }
        else
            head = ll->next;
        ll->next = ll->prev = NULL;
        ll->recalc();
        cnt--;
        recalc();
    }
    void add_after( T * item, T * existing ) {
        validate();
        LListLinks<T> * exll = & existing->links[instance];
        LListLinks<T> * itll = & item->links[instance];
        exll->validate();
        itll->validate();
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
        {
            itll->next->links[instance].prev = item;
            itll->next->links[instance].recalc();
        }
        else
            tail = item;
        exll->recalc();
        itll->recalc();
        cnt++;
        recalc();
    }
    void add_before( T * item, T * existing ) {
        validate();
        LListLinks<T> * exll = & existing->links[instance];
        LListLinks<T> * itll = & item->links[instance];
        exll->validate();
        itll->validate();
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
        {
            itll->prev->links[instance].next = item;
            itll->prev->links[instance].recalc();
        }
        else
            head = item;
        exll->recalc();
        itll->recalc();
        cnt++;
        recalc();
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
        validate();
        return head;
    }
    T * get_tail ( void ) {
        validate();
        return tail;
    }
    T * get_next( T * x ) {
        validate();
        if ( !onthislist( x ))
            DLL2BAIL(( "ERROR get_next ITEM is not on this list!\n" ));
        x->links[instance].validate();
        return x->links[instance].next;
    }
    T * get_prev( T * x ) {
        validate();
        if ( !onthislist( x ))
            DLL2BAIL(( "ERROR get_prev ITEM is not on this list!\n" ));
        x->links[instance].validate();
        return x->links[instance].prev;
    }
};

// to use LListHash, type T must have a companion comparator class
// which provides hints about how to do hashing on the item. This
// comparator class must have the following static members:
//
//    static int hash_key( T * item ) 
//    static int hash_key( Hash_Key_Type key )
//    static bool hash_key_compare( T * item, Hash_Key_Type key )
//

template <class T, class Hash_Key_Type,
          class Hash_Key_Comparator, int instance>
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
        hash = LOGNEW theLListHash[ hashsize ];
        count = 0;
    }
    ~LListHash ( void ) {
        for ( int i = 0; i < hashsize; i++ )
            if ( hash[i].get_cnt() != 0 )
                DLL2BAIL(( "LLISTHASH destructor: hash not empty!\n" ));
        delete[] hash;
    }
    T * find    ( Hash_Key_Type key ) {
        int h = hashv( Hash_Key_Comparator::hash_key( key ) & 0x7fffffff );
        T * ret;
        for ( ret = hash[h].get_head(); ret; ret = hash[h].get_next(ret))
            if ( Hash_Key_Comparator::hash_key_compare( ret, key ))
                break;
        return ret;
    }
    int get_cnt( void ) { return count; }
    void add( T * t ) {
        count++;
        int h = Hash_Key_Comparator::hash_key( t ) & 0x7fffffff;
        hash[ hashv( h ) ].add( t );
    }
    void remove( T * t ) {
        count--;
        int h = Hash_Key_Comparator::hash_key( t ) & 0x7fffffff;
        hash[ hashv( h ) ].remove( t );
    }
    bool onlist( T * t ) {
        int h = Hash_Key_Comparator::hash_key( t ) & 0x7fffffff;
        return hash[ hashv( h ) ].onlist( t );
    }
    bool onthislist( T * t ) {
        int h = Hash_Key_Comparator::hash_key( t ) & 0x7fffffff;
        return hash[ hashv( h ) ].onthislist( t );
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

template <class T, 
          class Hash_Key_Type, class Hash_Key_Comparator,
          int hash_instance, int lru_instance>
class LListHashLRU {
private:
    LListHash <T, Hash_Key_Type, Hash_Key_Comparator, hash_instance> hash;
    LListLRU  <T, lru_instance > lru;
public:
    LListHashLRU ( int hashsize ) : hash( hashsize ) { }
    ~LListHashLRU( void ) { }
    T * find( Hash_Key_Type key ) { return hash.find( key ); }
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

#if DLL2_INCLUDE_BTREE
#include "dll2_btree.H"
#endif

#endif /* __DLL2_H_ */
