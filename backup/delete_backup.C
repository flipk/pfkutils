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

/** \file delete_backup.C
 * \brief Defines method for deleting all data in a database associated with
 *        a particular backup.
 * \author Phillip F Knaack 
 */

#include "database_elements.H"
#include "params.H"
#include "protos.H"

#include <FileList.H>

#include <stdlib.h>

/** delete a backup from a backup database.
 * 
 * @param baknum The backup ID number of the backup to delete.
 */
void
pfkbak_delete_backup ( UINT32 baknum )
{
    printf("Sorry, this functionality is not yet implemented.\n");
    /** \todo support deleting backup from a database. */
}
