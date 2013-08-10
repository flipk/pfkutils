
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

extern int treesync_verbose;

void     treesync_analyze     (char * dir1, TSFileEntryList * fel1,
                               char * dir2, TSFileEntryList * fel2 );
void     treesync_copy_file   (char *fromroot, char *fromfile,
                               char *toroot,   char *tofile    );
void     treesync_delete_file (char *root, char *file);
int      treesync_create_dirs (char * path);
void     treesync_display_md5 (char *path, UINT8 *md5);
void     treesync_calc_md5    (char *root_dir, char *relpath,
                               UINT8 * hashbuffer );
Btree *  open_treesync_db     (char *dir);
void     treesync_update_db   (char *root_dir, Btree * db,
                               TSFileEntryList * fel );

