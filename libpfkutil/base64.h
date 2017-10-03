
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

#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef __cplusplus
extern "C" {
#endif

int b64_is_valid_char( unsigned char c );

/* return 4 if ok, 0 if not ok */
int b64_encode_quantum( unsigned char * in3, int in_len, unsigned char * out4 );

/* return length of bytes decoded, or 0 if not ok */
int b64_decode_quantum( unsigned char * in4, unsigned char * out3 );


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __BASE64_H__ */
