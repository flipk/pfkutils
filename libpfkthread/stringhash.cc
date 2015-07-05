
/**
 * \file stringhash.cc
 * \brief utility function for hashing strings
 * \author Phillip F Knaack <pfk@pfk.org>

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

#include "stringhash.h"

// #include <stdio.h>

/* djb2 string hash algorithm */

int
string_hash( const char * string )
{
    unsigned int ret = 5381;
    unsigned int c;

    while ((c = (unsigned int) *string++) != 0)
        ret = ((ret << 5) + ret) + c;  /* hash * 33 + c */

    return (int) ret;
}
