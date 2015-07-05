
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

#ifndef __PFKMATH_H__
#define __PFKMATH_H__

#ifdef __cplusplus
extern "C" {
#endif

enum m_math_retvals {
	M_MATH_OK = 0,
	M_MATH_SYNTAX,
	M_MATH_NOFLAGS,
	M_MATH_STACKFULL,
	M_MATH_TOOMANYARGS,
	M_MATH_EMPTYSTACK,
	M_MATH_STACKNOTONE,
	M_MATH_REGERR,
	M_MATH_TOOFEWARGS,
	M_MATH_OVERFLOW,
	M_MATH_DIVIDE_BY_ZERO,
	M_MATH_PARSEVALUEERR,
	M_MATH_OP1ERR,
	M_MATH_OP2ERR
};

#define M_INT64  unsigned long long

/* when this returns something other than M_MATH_OK,
   "result" is actually a pointer to a descriptive error string.
   also, if you specify "flags" as null, it will not allow flags
   as arguments and will return M_MATH_NOFLAGS if you specify
   flag arguments but flags==NULL.
*/

enum m_math_retvals m_do_math( int argc, char ** argv,
                               M_INT64 *result, int *flags );

/* this returns a static global pointer, so it does not have
   to be freed, however it doesn't survive two successive calls. */
char * m_dump_number( M_INT64 num, int base );

/* this parses the string in the given base and either returns
   zero with a valid number, or returns -1 with no valid number.
   0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz  */
int    m_parse_number( M_INT64 * result, char * string, int len, int base );

#ifdef __cplusplus
};
#endif

#endif /* __PFKMATH_H__ */
