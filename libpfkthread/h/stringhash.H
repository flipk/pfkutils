
/**
 * \file stringhash.H
 * \brief helper function for hashing strings into integers
 * \author Phillip F Knaack <pknaack1@netscape.net>

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

#ifndef __STRINGHASH_H__
#define __STRINGHASH_H__

/** hash a string into an integer.
 * this method uses the djb2 string hash algorithm to
 * generate an integer from a string that is useful for
 * searching a hash based on a string. the algorithm attempts
 * to reasonably-space ascii strings throughout the hash space.
 * \param string   the string to hash
 * \return a positive integer hash of the string
 */
int string_hash( const char * string );

#endif /* __STRINGHASH_H__ */
