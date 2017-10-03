
#include "dll2.H"
#include "pk-md5.h"
#include "types.H"

#include <string.h>

void scan_start( void );
void scan_finish( bool success );

enum { FFE_LIST, FFE_HASH, FFE_NUMLISTS };

enum FFE_status {
    FFE_NF, // note this never appears in an FFE, used by trader
    FFE_NEW,
    FFE_DELETED,
    FFE_SAME,
    FFE_CHANGED,
    FFE_NUM_STATUSES
};

struct Final_File_Entry {
    LListLinks <Final_File_Entry> links[FFE_NUMLISTS];
    MD5_DIGEST whole_digest; // not valid if status == deleted
    MD5_DIGEST * digests;    // not valid if status == deleted
    UINT64  length;          // not valid if status == deleted
    UINT32  num_segments;    // not valid if status == deleted
    UINT32  next_digest;     // not valid if status == deleted
    FFE_status status;
    char  filename[0]; // must be last, since it is variable length
    Final_File_Entry( void ) { digests = NULL; }
    ~Final_File_Entry( void ) { if ( digests ) delete[] digests; }
    static void * operator new( size_t __unused, int l ) {
        return (void*) new char[ sizeof(struct Final_File_Entry) + l ];
    }
    static void operator delete( void * _ptr ) {
        char * ptr = (char*) _ptr; delete[] ptr;
    }
};

class Final_File_Entry_Hash_1 {
public:
    static int hash_key( Final_File_Entry * item ) {
        return hash_key( item->filename );
    }
    static int hash_key( char * _key ) {
        char * key = _key;
        int sum = 0;
        while ( *key ) {
            sum <<= 1;
            sum += *key;
            key++;
        }
        return sum;
    }
    static bool hash_key_compare( Final_File_Entry * item, char * key ) {
        return (strcmp( item->filename, key ) == 0);
    }
};

class Final_File_List {
    static const int hash_size = 3000;
    LList <Final_File_Entry,FFE_LIST> lst;
    LListHash <Final_File_Entry,  char*,
               Final_File_Entry_Hash_1, FFE_HASH> hash;
public:
    Final_File_List( void ) : hash( hash_size ) { }
    void add( Final_File_Entry * t ) { lst.add( t );  hash.add( t ); }
    void remove( Final_File_Entry * t ) { lst.remove( t ); hash.remove( t ); }
    Final_File_Entry * find( char * fname ) { return hash.find(fname); }
    Final_File_Entry * get_head(void) { return lst.get_head(); }
    Final_File_Entry * get_next(Final_File_Entry *x)
    { return lst.get_next(x); }
    Final_File_Entry * dequeue_head( void ) {
        Final_File_Entry * ret = lst.dequeue_head();
        if ( ret ) hash.remove( ret );
        return ret;
    }
};

extern Final_File_List * scan_list;
