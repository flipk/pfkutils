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

/** \file delete_backup.cc
 * \brief Defines method for deleting all data in a database associated with
 *        a particular backup.
 * \author Phillip F Knaack 
 */

#include "database_elements.h"
#include "params.h"
#include "protos.h"
#include "FileList.h"

#include <stdlib.h>

/** delete a backup from a backup database.
 * 
 * @param baknum The backup ID number of the backup to delete.
 */
void
pfkbak_delete_backup ( uint32_t baknum )
{
    printf("Sorry, this functionality is not yet implemented.\n");
    /** \todo support deleting backup from a database. */
}
