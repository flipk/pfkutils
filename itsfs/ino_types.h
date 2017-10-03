
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

#ifndef __INO_TYPES_H_
#define __INO_TYPES_H_

typedef enum {
    INODE_REMOTE  = 1,
    INODE_VIRTUAL = 2
} inode_type;

typedef enum {
    INODE_DIR   = 1,
    INODE_FILE  = 2,
    INODE_LINK  = 3  /* (symlink)  */
} inode_file_type;

#endif /* __INO_TYPES_H_ */
