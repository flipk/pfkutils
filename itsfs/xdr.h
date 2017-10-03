
/*
    This file is part of the "pkutils" tools written by Phil Knaack
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

#ifndef __XDR_H_
#define __XDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mytypes.h"

typedef struct {
    int encode_decode;
    uchar * data;
    int position;
    int bytes_left;
} XDR;

#define XDR_ENCODE 1
#define XDR_DECODE 2
#define XDR_FREE   3

typedef bool_t (*xdrproc_t)(XDR *,...);

typedef struct {
    enum_t val;
    xdrproc_t proc;
} xdrunion_table_t;

bool_t myxdr_void      ( XDR * xdr );
bool_t myxdr_bytes     ( XDR * xdr,
                         uchar ** data, u_int * data_len, u_int max_data );
bool_t myxdr_opaque    ( XDR * xdr,
                         uchar * data, u_int size );
bool_t myxdr_reference ( XDR * xdr,
                         uchar ** data, u_int size, xdrproc_t method );
bool_t myxdr_pointer   ( XDR * xdr,
                         uchar ** data, u_int size, xdrproc_t method );
bool_t myxdr_string    ( XDR * xdr,
                         uchar ** str, u_int max );
bool_t myxdr_u_int     ( XDR * xdr,
                         u_int * v );
bool_t myxdr_int       ( XDR * xdr,
                         int * v );
bool_t myxdr_enum      ( XDR * xdr,
                         enum_t * v );
bool_t myxdr_bool      ( XDR * xdr,
                         bool_t * v );
bool_t myxdr_union     ( XDR * xdr,
                         enum_t * disc, uchar * union_ptr,
                         xdrunion_table_t * tab, xdrproc_t def );

#ifdef __cplusplus
}
#endif

#endif /* __XDR_H_ */
