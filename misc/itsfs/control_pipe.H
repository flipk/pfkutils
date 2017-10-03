
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

#ifndef __CONTROL_PIPE_H_
#define __CONTROL_PIPE_H_

#if 0
enum {
    CONTROL_PIPE_EXIT,   // impossible
};

union CONTROL_PIPE_REQ {
    struct {
        uchar type;   // CONTROL_PIPE_EXIT
    } exit;
};

union CONTROL_PIPE_REPL {
    struct {
        uchar type;   // CONTROL_PIPE_EXIT
    } exit;
};
#endif

class Control_Pipe {
    uchar * data;
    int     datalen;
public:
    Control_Pipe( void );
    ~Control_Pipe( void );
    int write  ( uchar * buf, int  length );
    int read   ( uchar * buf, int &length );
    int len    ( void ) { return datalen; }
};

#endif /* __CONTROL_PIPE_H_ */
