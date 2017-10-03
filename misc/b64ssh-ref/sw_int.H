
#ifndef __SLIDING_WINDOW_INTERNAL_H__
#define __SLIDING_WINDOW_INTERNAL_H__

#include "pk_threads.H"
#include "dll2.H"

enum { DATA_BUFFER_LIST, 
       DATA_BUFFER_HASH, 
       DATA_BUFFER_DLLS };

struct DataBufferEntry {
    LListLinks<DataBufferEntry> links[DATA_BUFFER_DLLS];
    int timer_id;
    unsigned char seqno;
    unsigned char delayed_free_age;
    DataBuffer * data;
};

class DataBufferEntrySeqnoCompare {
public:
    static int hash_key( DataBufferEntry * item ) {
        return item->seqno;
    }
    static int hash_key( unsigned char key ) { return (int)key; }
    static bool hash_key_compare( DataBufferEntry * item, int key ) {
        return (int)item->seqno == key; 
    }
};

typedef LList<DataBufferEntry,DATA_BUFFER_LIST> DataBufferQueue;
typedef LListHash<DataBufferEntry,unsigned char,
                  DataBufferEntrySeqnoCompare,
                  DATA_BUFFER_HASH> DataBufferSeqnoHash;

class DataBufferList {
    DataBufferQueue     q;
    DataBufferSeqnoHash h;
public:
    void    add( DataBufferEntry * x ) { q.   add(x); h.   add(x); }
    void remove( DataBufferEntry * x ) { q.remove(x); h.remove(x); }
    bool onthislist( DataBufferEntry * x ) { return q.onthislist(x); }
    DataBufferEntry * get_head(void) { return q.get_head(); }
    DataBufferEntry * get_next(DataBufferEntry * i) { return q.get_next(i); }
    DataBufferEntry * find( unsigned char seqno ) { return h.find(seqno); }
    int get_cnt(void) { return q.get_cnt(); }
};

#endif /* __SLIDING_WINDOW_INTERNAL_H__ */
