
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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

#if 1
#define LOGNEW new
#define MALLOC(size) malloc( size )
#define STRDUP(str) strdup( str )
#else

#ifdef __cplusplus

void * operator new     ( size_t s, char * file, int line );
void * operator new[]   ( size_t s, char * file, int line );

#define LOGNEW new(__FILE__,__LINE__)

extern "C" {

#endif

void * malloc_record( char * file, int line, int size );
char * strdup_record( char * file, int line, char *str );
#define MALLOC(size) malloc_record( __FILE__, __LINE__, size )
#define STRDUP(str) strdup_record( __FILE__, __LINE__, str )

#ifdef __cplusplus
};
#endif
#endif /* if 0 */
