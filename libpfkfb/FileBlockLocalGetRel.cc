
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

/** \file FileBlockLocalGetRel.cc
 * \brief Everything dealing with fetching, releasing, and flushing blocks.
 * \author Phillip F Knaack
 */

/** \page FileBlockAccessR FileBlock Access Methods: Retrieval

The second phase is to actually access that region.  This is done with two
more methods, FileBlockInterface::get and FileBlockInterface::release.  The
first takes an AUID number created by the alloc method, and returns a
FileBlock, which contains a memory buffer the user can read and modify.
The release method takes this FileBlock object back, and ensures the memory
buffer's contents are synchronized with the actual disk file.

See the section \ref Templates for an example of the access methods.

Next: \ref AUNMGMT

*/

#include "FileBlockLocal.h"

#include <stdlib.h>


//virtual
FileBlock *
FileBlockLocal :: get( FB_AUID_T auid, bool for_write )
{
    FB_AUN_T aun;

    aun = translate_auid(auid);

    if (aun == 0)
        return NULL;

    FileBlockInt * fb = get_aun(aun,for_write);

    if (fb)
    {
        fb->set_auid( auid );
    }

    return fb;
}

//virtual
void
FileBlockLocal :: release( FileBlock * _fb, bool dirty )
{
    FileBlockInt * fb = (FileBlockInt*)_fb;

    active_blocks.remove(fb);

    bc->release(fb->get_bcb());

    delete fb;
}

//virtual
void
FileBlockLocal :: flush(void)
{
    fh.release();
    bc->flush();
    fh.get();
// this takes too long to do all the time.
//    validate(false);
}
