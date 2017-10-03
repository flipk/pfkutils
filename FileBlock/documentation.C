
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

/** \file documentation.C
 * \brief container for doxygen documentation.
 *
 * See also: \ref FileBlockBSTExample
 *
 * \author Phillip F Knaack
 */

/** \mainpage FileBlock Interface and Btree

The purpose of the FileBlockInterface is to manage allocation of file
space within a file.  An excellent analogy to the FileBlockInterface 
object are the standard unix functions \em malloc and \em free, except that
it is for file space rather than memory space.

 \section FileBlockObjects FileBlock component objects

The FileBlock interface is composed of a number of lower-level object
types.  Click on the following links to read about each of them.

<ul>
<li> \ref PageIO (see classes PageIO and PageIOFileDescriptor)
<li> \ref PageCache (see classes PageCachePage and PageCache)
<li> \ref BlockCache (see classes BlockCacheBlock and BlockCache)
<li> \ref FileBlock (see classes FileBlock, FileBlockInterface, FileBlockLocal)
<li> \ref FileBlockLocalFileFormat
</ul>

\section BtreeObjects Btree component objects

<ul>
<li> \ref BtreeStructure (see Btree and BTDatum)
<li> \ref BtreeInternalStructure (see BtreeInternal, _BTInfo,
          BTNodeItem, _BTNodeDisk, BTKey, BTNode, BTNodeCache)
</ul>

\section TemplatesObjects Templates

<ul>
<li> \ref Templates (see templates FileBlockT, BlockCacheT, and BTDatum)
</ul>


Author: Phillip F Knaack <pknaack1@netscape.net>

<pre>
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
</pre>

*/


/** \page PageIO PageIO object

The lowest layer is a derived object from PageIO.  This object knows
only how to read and write PageCachePage objects, whose body is of
size PageCache::PAGE_SIZE.  An example implementation of PageIO is the
object PageIOFileDescriptor, which uses a file descriptor (presumably
an open file) to read and write offsets in the file.

If the user wishes some other storage mechanism (such as a file on a
remote server, accessed via RPC/TCP for example) then the user may
implement another PageIO backend which performs the necessary
interfacing.  This new PageIO object may be passed to the PageCache
constructor.

Next: \ref PageCache

*/


/** \page PageCache PageCache object

The PageIO interface is consumed by a PageCache object.  This is a
cache of PageCachePage objects up to a maximum number.  The method
PageCache::get() will retrieve a page from the cache if it is in
cache, or fetch it from the PageIO interface if it is not (and add it
to the cache).  A page returned by PageCache::get() is in the \c locked
state with a \c reference \c count greator than 0.  When the caller is
done using the page object, it must call PageCache::release() to
return the page to the cache.  This decreases the reference count on
the page.  When the PageCache::flush() method is called, all pages
currently in cache are synced up with the backend storage, by invoking
the PageIO interface to write any pages marked as dirty.

If the number of pages cached in PageCache reaches the maximum, then a
true \c Least-Recently-Used (LRU) algorithm is used to determine which
old page to remove from cache.  If the oldest page is clean (in sync
with the PageIO backend storage) then the memory is simply freed.  If
the oldest page is dirty, then PageIO::put_page() is invoked prior to
freeing the page memory.

When the \c for_write flag is set on a PageCache::get() operation, this
indicates that the user has no intention of using any data currently
stored in the page, and instead intends to write data to the whole
page.  This results in an optimization where the PageIO::get_page()
method is not called, instead an empty PageCachePage is built and
returned to the user.  (Why read a page from the file if you're just
going to throw the data away and write the whole page with new data
anyway?)

\note The maximum count only applies to unlocked pages.  A locked
page does not count against the limit.

Be sure to call PageCachePage::mark_dirty() to ensure that flushes are
written to the file properly.

Next: \ref BlockCache 

*/

/** \page BlockCache BlockCache object

The PageCache object is consumed by the BlockCache object.  This
object abstracts the concept of arbitrarily-sized blocks in a file.
When the BlockCache::get() method is called, it determines how many
pages are required to satisfy the request.  If the entire request is
within a single page, the returned object is optimized by returning a
pointer directly into the PageCachePage object.  Since pages are
generally pretty large, most BlockCache operations are thus within a
single page.  Thus, this optimization results in dramatic zero-copy
speedup.  If the request spans more than one page, then a separate
memory buffer is allocated, and the block object maintains references
to all pages.

Various optimizations are also done when the \c for_write parameter is
specified.  Obviously if the user is requesting only a portion of a
page, then for_write cannot be passed to PageCache, because this could
trash the remainder of the page which is not being modified.  However
in multi-block fetches, for_write does have an effect if complete
pages are involved in the block.

When the BlockCache::flush() method is invoked, it will eventually
call PageCache::flush().  This will automatically synchronize all
single-page blocks with the data file; however for multi-page blocks,
some additional work must be done before flushing the PageCache.  All
portions of the multi-page blocks are copied to their respective pages
prior to calling PageCache::flush().

Be sure to call BlockCacheBlock::mark_dirty() to ensure that flushes are
written to the containing pages properly.

\note While it is legal for PageCache users to call PageCache::get()
on the same page twice (because of internal reference counts, etc),
this is \b NOT legal for BlockCache blocks.  Undefined behavior and data
corruption will occur if the same or overlapping blocks are requested
from BlockCache at the same time.

Next: \ref FileBlock

*/

/** \page FileBlock FileBlock object

This is the guts and core of file space allocation management.

 \section FBDefinitions FileBlock term definitions

<ul>
<li> AU : Allocation Unit

This is the fundamental unit of accessing a disk.  An AU is 32 bytes in size.

<li> AUN : Allocation Unit Number (FB_AUN_T)

This is an identifer for an AU which identifies the AU by its position in
the file.  Since an AU is 32 bytes in size, the position in the file can
be calculated from an AUN:  pos=(AUN*32).

This data type (FB_AUN_T) is completely internal to the FileBlockLocal 
implementation, and is not available to applications.

<li> AUID : Allocation Unit Identifier (FB_AUID_T)

This is also an identifer of an AU, but this identifier is completely 
independent of the position in the file.  In order to allow the FileBlock
API to perform transparent compactions of the file, but still allow the 
application to store identifiers in their own data structures, it is 
necessary to separate the identifier from the position in the file.  Then,
the API can feel free to move a unit to another place in the file without
informing the application that it must change its identifier.

There is a translation mechanism in the file providing a lookup table from
AUID to AUN so that a block can be located quickly.  If the API decides to
move a unit in order to compact the file, the lookup table is changed but
the application continues to use the same AUID.

</ul>

\todo populate this section

 \section AUNMGMT  AUN Management

 \section AUNBuckets  AUN Bucket Lists

 \section AUIDMGMT  AUID Management

 \section AUIDTable  AUID Three-level table

 \section AUIDStack  AUID Free Stack

 (auid stack table)

 \section FileBlockAccess FileBlock Access Methods

There are two separate phases in accessing a FileBlock file.  First there
is allocation management, in which there are three methods:
 FileBlockInterface::alloc, FileBlockInterface::realloc, and
FileBlockInterface::free.

These functions deal only in FileBlock AUID numbers.  An AUID does not
exist until an alloc call, and ceases to exist after a free call.
When allocating an AUID, the size must also be specified.  After the
alloc call, there now exists a region within the disk file of the
specified size which is associated with this ID.

The realloc method is basically a free followed by another alloc of
the new size, except that the new allocation has the same AUID number as
the old one.  Note that it will copy the data from the old region to the
new region; if the new size is smaller, the data is truncated, but if the
new size is bigger, the new space at the end of the allocation is memset
to zeroes.

The second phase is to actually access that region.  This is done with two
more methods, FileBlockInterface::get and FileBlockInterface::release.  The
first takes an AUID number created by the alloc method, and returns a
FileBlock, which contains a memory buffer the user can read and modify.
The release method takes this FileBlock object back, and ensures the memory
buffer's contents are synchronized with the actual disk file.

See the section \ref Templates for an example of the access methods.

 \section Compacting

\note At this time the Compaction algorithm does not yet exist.

One possible implementation requires halting access to the file and
creating an entirely new file, walking the original file in block
order and simply tail-allocating from the new file, guaranteeing the
new file is completely packed.  While this guarantees perfect packing
and excellent performance (due to the linear nature of both the reader
and the writer) it unfortunately requires the access to the file to be
halted-- an unacceptable step for any program intended to run
continuously for long periods.

It may be a long time (if ever) before a compaction algorithm is
designed, due to the recent addition of memory-btrees in the
free-bucket list; this addition will dramatically reduce the long-term
fragmentation of the file.

\todo design and implement the compaction algorithm.

See also: \ref FileBlockBSTExample

Next: \ref FileBlockLocalFileFormat

*/

/** \page FileBlockLocalFileFormat FileBlockLocal File Format

\todo more needed here

\note All 4-byte or 8-byte numeric fields are encoded in big-endian format.

\section FileBlockLocalFileFormatFileHeader File Header

\section FileBlockFileInfoBlock File Info Blocks

Following this are 64 4-byte units identifying AUIDs where file-
information blocks exist.  The value 0 means no information block is
populated in this slot.  A nonzero value indicates the presence of a
FileInfoBlockId structure.  This structure is 132 bytes in size.  The
first 4 bytes are a block id identifying the location of the user's 
info block.  The remaining 128 bytes are a character string uniquely
identifying the info block.

To enter a file block association into the file, the user specifies
the unique string and provides an AUID of the info block when
calling FileBlockInterface::set_data_info_block.

To retrieve this information later, the user specifies the unique string
and then calls FileBlockInterface::get_data_info_block.

If this information is no longer needed, the user should delete the 
association by calling FileBlockInterface::del_data_info_block.

\section FileBlockLocalFileFormatFuture The future of the FileBlock interface

There is room for improvement in this API.  

\todo more needed here

A program crash while the file is open can unfortunately be rather 
catastrophic as well.

Next: \ref BtreeStructure

 */

/** \page BtreeStructure BTREE File Interface

The BTREE is a data storage/retrieval mechanism.  It uses a "key" data
structure to index the data, so that if the proper key is provided,
the corresponding data can be located quickly.

At the lowest conceptual level, a "key" and its corresponding "data"
are just sequences of bytes of arbitrary length and contents.

Keys must be unique.  If a "put" is done on two key/data pairs where
the keys are the same number of bytes and have the same contents, the
data portion of the second "put" will replace the data of the first in
the file.

The "key" and "data" units are specified with UCHAR pointers and an int
specifying a length.  A UCHAR and int together are known as a "datum".

To retrieve data from a Btree, construct a key datum with the proper
contents, and call the "get" method.  It will return a data datum which
has been populated with the matching data.
 
\todo document btree

\section BtreeTemplate BTREE Template types

Next: \ref BtreeInternalStructure

 */

/** \page BtreeInternalStructure BTREE Internal Structure

\todo document btree internals

\section BtreeNodeOnDisk Btree node, on-disk layout

The BTREE node, on disk, has the following layout:

<ul>
<li> UINT32_t magic
     <ul>
     <li> which must be set to _BTNodeDisk::MAGIC
     </ul>
<li> UINT16_t numitems
     <ul>
     <li> the bottom 14 bits (13-0) of this value encodes the number of items
     	  stored in this node.  this can be no greator than order-1, and
	  must be greator than or equal to order/2 (except in the root node,
	  where the number of items can be anywhere from 0 to order-1).
     <li> bit 14 indicates if this is a root node.  obviously this can be
     	  determined by the fact that the BTInfo points to this node, but
	  it is an extra sanity check.
     <li> bit 15 indicates if this is a leaf node.  in this case the pointers
     	  should not be followed, and inserts should take place here.
     </ul>
<li> BTNodeItem items[order-1]:
     <ul>
     <li> FB_AUID_t ptr
     	  <ul>
	  <li> this is a FileBlock ID number of a pointer to a child node.
	  </ul>
     <li> UINT16_t keystart
     <li> UINT16_t keysize
     	  <ul>
	  <li> these indicate where the key data for this item begin within
	       the keydata element below.
     	  </ul>
     <li> FB_AUID_t data
     	  <ul>
	  <li> this is a FileBlock ID number of the data corresponding to
	       the key above.
	  </ul>
     </ul>
<li> FB_AUID_t ptr
     <ul>
     <li> the number of ptrs in a node is \em order, however the number
     	  of key/data items in a node is \em order-1.  so there is an
	  extra ptr at the end of the array to round out the node.
     <li> since \em ptr is the first element of the items array, it thus
     	  works to access items[order].ptr (even though the dimension of the
	  array is order-1), to access this extra ptr.
     </ul>
<li> UCHAR keydata[]
     <ul>
     <li> this is a variable-length array of data where the key data for
     	  all the items begins.  the \em keystart and \em keysize elements
	  indicate offsets and sizes in this array.  all of the key data
	  are contiguous in this array, thus the total size of this
	  array is items[order-2].keystart + items[order-1].keysize.
     </ul>
</ul>

Next: \ref Templates

*/

/** \page Templates Template Data Types

Previous versions of the file access methods have been rather clumsy
to use due partially to lack of documentation, partially due to
inconsistent use of data types, and partially due to an overly complex
interface.

Consider that in previous iterations, the get_block methods returned a
UCHAR* which had to be cast to the relevant data type.  Also, there
was a separate UINT32 "magic" value which the application had to
store, to be used when a block was released ("unlocked" in the old
terminology).  (The "magic" was actually a pointer to an internal data
type describing the block, to assist in synchronizing the block back
to cache and such.  This information is now contained in the
PageCachePage object for the PageCache interface, the BlockCacheBlock
object for the BlockCache interface, the FileBlock object for the
FileBlockInterface interface, and the BTDatum for Btree interface.

First, lets see some code to change an employee's gradelevel using the
old API:

<pre>
    bool change_employee_gradelevel( FileBlockNumber * fbn,
                                     UINT32 blockid,
                                     GRADE new_grade )
    {
	int size;
	UINT32 magic;
	Employee * emp;
	emp = (Employee *) fbn->get_block( blockid, &size, &magic );
        if (!emp)
        {
            fprintf(stderr, "employee id %#x not found!\n", blockid);
            return false;
        }
        emp->gradelevel = new_grade;
        fbn->unlock_block( magic, true );
        return true;
    }
</pre>

Note that the API user must store a "magic" number to be passed back
to the unlock method, once the change has been made; also note the
type cast in the return value, and the unused "size" parameter that
must be specified.

Now contrast this with the new API (without using the template types):

<pre>
    bool change_employee_gradelevel( FileBlockInterface * fbi,
                                     FB_AUID_T blockid,
                                     GRADE new_grade )
    {
        FileBlock * fb = fbi->get(blockid);
        if (!fb)
        {
            fprintf(stderr, "employee id %#x not found!\n", blockid);
            return false;
        }
        Employee * emp = (Employee *) fb->get_ptr();
        emp->gradelevel = new_grade;
        fbi->release(fb,true);
        return true;
    }
</pre>

The user has a new data type "FileBlock" which contains information the
user can have if he wants it (offset in file, block id, block size) but
the user does not have to access those fields or provide a place to put
them if he doesn't care to use them.  This technique still requires an
explicit typecast.

Finally, compare this with the use of the new template type:

<pre>
    bool change_employee_gradelevel( FileBlockInterface * fbi,
                                     FB_AUID_T blockid,
                                     GRADE new_grade )
    {
	FileBlockT &lt;Employee&gt;  emp(fbi);
	if (!emp.get(blockid))
	{
	    fprintf(stderr, "employee id %#x not found!\n", blockid);
            return false;
	}
	emp.d->gradelevel = new_grade;
	emp.mark_dirty();
        return true;
    }
</pre>

There is no longer a need for a typecast. All the data is still available
in emp.fb, but the user no longer needs to keep anything but the one single
variable (emp).  Also, the user does not need to explicitly call a release
or unlock method when done, because the descructor of the FileBlockT type
does this for him.  (A "release" method is available, in case there are
timing considerations, but it is not usually necessary.)  Also, if a second
"get" is called on this object, this is an implicit release on the first
datum, so if the first datum had been modified and marked dirty, this second
get would cause the first data to be properly flushed back to the file.

This interface is considerably simpler than the previous interaces.

A similar discussion can be found for the BTDatum types, on the Btree page.

 */
