/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

/** \file FileBlockLocalGetRel.cc
 * \brief Everything dealing with fetching, releasing, and flushing blocks.
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
