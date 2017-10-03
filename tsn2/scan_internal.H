
#define DEBUG_GET_PUT          0
#define DEBUG_DUMP_DB          0
#define DEBUG_CALC_DIGESTS     0
#define DEBUG_PROC_DIRS        0
#define DEBUG_PROC_FILES       0
#define DEBUG_TRIM             0
#define DEBUG_NEW_FILES        0
#define DEBUG_UPDATES          0
#define DEBUG_DELETES          0
#define DEBUG_CHANGED_DIGEST   0
#define DEBUG_PRINT_FFE_LIST   1

// btkeys : two bytes for 'prefix', remainder is filename (minus NUL)
// prefix values:
//   0000 for touch rec
//   0001 filename
//   0002-ffff for md5 digests
//         (first digest is whole-file digest)

struct db_file_entry {
    UINT64_t  length;
    UINT32_t  mtime;
    UINT32_t  touch;
    UINT32_t  last_scan;
    UINT32_t  num_segments;  // NB: Top bit stolen for 'new' entries.
};

// key : 0002-ffff filename
// data: up to 32 md5 digests

#define DBFILE     "00TREEINFO.DB"
#define DBFILEBAK  "00TREEINFO.BAK.DB"
#define DBFILENEW  "00TREEINFO.NEW.DB"

#define DBFILEPREFIX "00TREE"
#define DBCACHEPARMS 1000
#define DIGESTS_PER_KEY 4
#define SEGMENT_SIZE 32768
#define CALC_NUM_SEGMENTS(sz)   ((sz==0)?0:(((sz-1) / SEGMENT_SIZE) + 1))

#define MAX_SEGMENTS (DIGESTS_PER_KEY * (1 << (8 * sizeof(UINT16) - 1)))

// a struct that contains only a file name and can be put in a list.
// this is probably more complex than it needs to be.
// there are 3 sources of new file_entry's:
//    a single NUL-terminated char*
//    two NUL-terminated char* that need a '/' between them
//        (i.e. a directory name plus directory entry)
//    a btkey entry (where we strip off the 2-byte header),
//         and where NUL must be added (db file does not store the NULs)

struct file_entry {
    LListLinks <file_entry> links[1];
    char name[0]; // must be last

    static file_entry * new_entry( char * ent ) {
        int l = strlen(ent) + 1;  // 1 for NUL
        return new(l) file_entry(ent,l);
    }
    static file_entry * new_entry( char * ent1, char * ent2 ) {
        int l1 = strlen(ent1);
        int l2 = strlen(ent2);
        // 1 for NUL, 1 for slash; l1 and l2 do not count NULs
        return new(l1+l2+2) file_entry(ent1,l1,ent2,l2);
    }
    static file_entry * new_entry( void * key, int keylen ) {
        // -2 for the 2byte number prefix, but note that
        // btkey does not include NUL so add 1.
        return new(keylen-2+1) file_entry( key, keylen );
    }
    static void operator delete( void * _ptr ) {
        char * ptr = (char*) _ptr;
        delete[] ptr;
    }

private:
    file_entry( char * _name, int l ) {
        // NB: l should include NUL
        memcpy( name, _name, l );
    }
    file_entry( char * _name1, int l1, char * _name2, int l2 ) {
        // NB: l1 and l2 should not include NUL
        char * ptr = name;
        memcpy( ptr, _name1, l1 );   
        ptr += l1;
        *ptr++ = '/';
        memcpy( ptr, _name2, l2 );
        ptr += l2;
        *ptr = 0;
    }
    file_entry( void * _key, int keylen ) {
        char * key = (char *) _key;
        // 2 for the 2byte number prefix;
        // NB: NUL is not included in btkey so we must add it.
        memcpy( name, key+2, keylen-2 );
        name[keylen-2] = 0;
    }
    static void * operator new( size_t __unused, int l ) {
        return (void*) new char[ sizeof(struct file_entry) + l ];
    }
};

typedef LList <file_entry,0> File_List;
