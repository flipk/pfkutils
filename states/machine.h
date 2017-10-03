/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

struct args {
    char * inputfile;
    char * headerfile;
    char * implfile;
    char * skelfile;
    int emit_base_class;
};

typedef struct {
    WENTLIST name;
    WENTLIST inputs;
    WENTLIST outputs;
    WENTLIST states;
    WENTLIST calls;
    VERBATIM * constructor_args;
    VERBATIM * constructor_code;
    VERBATIM * destructor_code;
    VERBATIM * startv;
    VERBATIM * startimplv;
    VERBATIM * datav;
    VERBATIM * endhdrv;
    VERBATIM * endimplv;
    int next_id_value;
    int entries;
    int bytes_allocated;
    int line_number;
    struct args * args;
} MACHINE;

extern MACHINE machine;

void init_machine( struct args * );
void destroy_machine( void );
struct wordentry * new_wordentry( char * w );

enum dump_type { DUMP_HEADER, DUMP_CODE, DUMP_SKELETON };
void dump_machine( enum dump_type );

void line_error( char * format, ... );
