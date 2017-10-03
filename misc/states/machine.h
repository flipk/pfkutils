
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
