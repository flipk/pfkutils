
#ifndef __REGEN_H_
#define __REGEN_H_

#include <string.h>
#include "dll2.H"
#include "types.H"

struct FileEntry {
    LListLinks <FileEntry>  links[1];
    enum { CHANGED, NEW, REMOVED, CONFLICT };

    int      type;   // one of the above 3 types
    UINT32   file_size;
    char     filename[1];

    FileEntry( char * fname, int _type, UINT32 _size ) {
        strcpy( filename, fname );
        type = _type;
        file_size = _size;
    }

    static void * operator new( size_t __sz, char * fname ) {
        int sz = strlen(fname) + sizeof(FileEntry);
        char * ret = new char[ sz ];
        return (void*) ret;
    }
    static void operator delete( void * _ptr ) {
        char * ptr = (char*) _ptr;
        delete[] ptr;
    }
};

typedef LList <FileEntry,0> FileList;

extern FileList  file_list;

void regenerate_database( void );

#endif /* __REGEN_H_ */
