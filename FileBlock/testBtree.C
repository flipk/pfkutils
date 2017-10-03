
#include "Btree.H"
#include "FileBlockLocal.H"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

struct empkey {
    UINT32_t id;
    //
    UCHAR * ptr(void) { return (UCHAR*) this; }
    int size(void) { return sizeof(*this); }
    static int max_size(void) { return sizeof(empkey); }
};

struct employee {
    UINT32_t id;
    char name[128];
    //
    UCHAR * ptr(void) { return (UCHAR*) this; }
    int size(void) { return sizeof(*this)-sizeof(name)+strlen(name)+1; }
    static int max_size(void) { return sizeof(employee); }
};

#define TEST_FILE "testfile.db"
#define MAX_BYTES (16*1024*1024)

class myPrintInfo : public BtreePrintinfo {
    int count;
public:
    myPrintInfo( void ) : BtreePrintinfo( 
        BtreePrintinfo::NODE_INFO     |
        BtreePrintinfo::BTREE_INFO    |
        BtreePrintinfo::KEY_REC_PTR   |
        BtreePrintinfo::DATA_REC_PTR  ) {
        count = 1;
    }
    /*virtual*/ ~myPrintInfo(void) { /* placeholder */ }
    /*virtual*/ bool print_item( UINT32 key_fbn, UINT32 data_fbn ) {
        printf( "%d: key %#x data %#x\n", count++, key_fbn, data_fbn );
        return true;
    }
    /*virtual*/ void print( const char * format, ... )
        __attribute__ ((format( printf, 2, 3 ))) {
        va_list ap;
        va_start(ap,format);
        vprintf(format,ap);
        va_end(ap);
    }
};
    
int
main()
{
    (void) unlink( TEST_FILE );

    int fd = open( TEST_FILE, O_RDWR | O_CREAT, 0644 );
    if ( fd < 0 )
    {
        fprintf(stderr, "open: %s\n", strerror(errno));
        return 1;
    }

    PageIO * pageio = new PageIOFileDescriptor(fd);
    BlockCache * bc = new BlockCache( pageio, MAX_BYTES );
    FileBlockLocal::init_file(bc);
    FileBlockInterface * fbi = new FileBlockLocal(bc);
    Btree::init_file( fbi, 5 );
    if (Btree::valid_file( fbi ))
        printf("file is a valid btree file\n");
    else
        printf("file is NOT a valid btree file\n");
    Btree * bt = Btree::open( fbi );


    myPrintInfo pi;
    bt->printinfo( &pi );



    delete bt;
    delete fbi;
    delete bc;
    delete pageio;
    close(fd);

    return 0;
}
