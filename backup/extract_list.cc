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

/** \file extract_list.cc
 * \brief specify a text file listing filenames that should be extracted
 *        from the backup.
 * \author Phillip F Knaack 
 */

#include "database_elements.h"
#include "params.h"
#include "protos.h"
#include "FileList.h"

#include <stdlib.h>

/** extract files from a backup using a list of filenames from a file.
 *
 * \todo This function is not implemented.
 *
 * @param baknum the backup ID number.
 * @param gen_num the generation number in the backup to extract.
 * @param list_file path to a file containing file names to extract.
 */
void
pfkbak_extract_list  ( UINT32 baknum,
                       UINT32 gen_num, char * list_file )
{
    printf("Sorry, this functionality is not yet implemented.\n");
    /** \todo modify extract.C to make a generic function for
     * extracting by file number, and then call it from both
     * extract_list and extract. */
}
