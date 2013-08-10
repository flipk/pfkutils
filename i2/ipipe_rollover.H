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

#ifndef __IPIPE_ROLLOVER_H__
#define __IPIPE_ROLLOVER_H__

#include "m.h"

class ipipe_rollover { 
    int current_size;
    int max_size;
    char * filename;
    int flags;
    char * ret;
    int counter;
public:
    ipipe_rollover( int _max_size, char * _filename, int _flags );
    ~ipipe_rollover( void );
    void check_rollover( int fd, int added );
    char * get_next_filename(void);
};

#endif /* __IPIPE_ROLLOVER_H__ */
