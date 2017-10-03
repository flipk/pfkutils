
#include "dll2.H"

enum { THING_LIST, THING_HASH, THING_NUM_LINKS };

struct thing {
    LListLinks <thing> links[THING_NUM_LINKS];
    int a;
};

struct thing_hash_1 {
    static int hash_key( thing * item ) {
        return item->a;
    }
    static int hash_key( int key ) {
        return key;
    }
    static bool hash_key_compare( thing * item, int key ) {
        return (item->a == key);
    }
};

typedef LListHash  <thing, int, thing_hash_1, THING_HASH>  thing_hash;
typedef LList      <thing,                    THING_LIST>  thing_list;

void add_entries( thing_hash * h, thing_list * l );
void del_entries( thing_hash * h, thing_list * l );
