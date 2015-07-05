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

/** \file get_info.cc
 * \brief Fetch database info from the database.
 * \author Phillip F Knaack
 */

#include "database_elements.h"
#include "params.h"
#include "protos.h"
#include "FileList.h"

#include <stdlib.h>

/** fetch database info from the database.
 *
 * @param info  pointer to caller's structure to populate when data is fetched.
 * @return  true if data was retrieved, false if some error occurred.
 */
bool
pfkbak_get_info( PfkBackupDbInfo * info )
{
    info->key.info_key.set((char*)PfkBackupDbInfoKey::INFO_KEY);

    if (!info->get())
    {
        fprintf(stderr, "unable to fetch info singleton\n");
        return false;
    }

    if (info->data.tool_version.v != PfkBackupDbInfoData::TOOL_VERSION)
    {
        fprintf(stderr, "tool version mismatch!!\n");
        return false;
    }

    return true;
}
