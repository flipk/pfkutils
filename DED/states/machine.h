/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
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
