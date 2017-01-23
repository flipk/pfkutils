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

/** \file FileBlock_iface.cc
 * \brief Common interface for FileBlock, which hides FileBlockLocal from 
 *        the application.
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
    else
        if (!valid_file(bc))
            return NULL;
    FileBlockInterface * fbi = open(bc);
    if (!fbi)
    {
        delete bc;
        return NULL;
    }
    return fbi;
}
