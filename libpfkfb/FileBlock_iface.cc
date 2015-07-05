
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

/** \file FileBlock_iface.cc
 * \brief Common interface for FileBlock, which hides FileBlockLocal from 
 *        the application.
 * \author Phillip F Knaack
 */

#include "FileBlock_iface.h"
#include "FileBlockLocal.h"

//static
FileBlockInterface *
FileBlockInterface :: open( BlockCache * bc )
{
    return new FileBlockLocal( bc );
}

//static
bool
FileBlockInterface :: valid_file( BlockCache * bc )
{
    return FileBlockLocal::valid_file( bc );
}

//static
void
FileBlockInterface :: init_file( BlockCache * bc )
{
    FileBlockLocal::init_file(bc);
}

//static
FileBlockInterface *
FileBlockInterface :: _openFile( const char * filename, int max_bytes,
                                 bool create, int mode )
{
    PageIO * pageio = PageIO::open(filename, create, mode);
    if (!pageio)
        return NULL;
    BlockCache * bc = new BlockCache( pageio, max_bytes );
    if (create)
        FileBlockInterface::init_file(bc);
    FileBlockInterface * fbi = open(bc);
    if (!fbi)
    {
        delete bc;
        return NULL;
    }
    return fbi;
}
