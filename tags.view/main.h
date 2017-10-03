
#include <stdio.h>

struct fileinfo {
    char * filename;
};

struct taginfo {
    int tagfileoffset;
    char tagname[0];
};

typedef struct {
    FILE * f;
    int numfiles;
    int numtags;
    struct fileinfo * files;
    struct taginfo ** tags;
}  TAGS_FILE;

void * xmalloc( int size );
void   xfree( void * );
extern int xmalloc_allocated;

TAGS_FILE * viewtags_tags_open( char * file );
void viewtags_tags_close( TAGS_FILE * );

#define MAXLINE 8192
#define MAXARGS 400
extern char viewtags_input_line[ MAXLINE ];
extern char * viewtags_lineargs[MAXARGS];

int viewtags_get_line( FILE * f );

void viewtags_display_tags( TAGS_FILE * tf, char * tagname );

/* return 0 when time to exit */
char * viewtags_input_tag( void );
