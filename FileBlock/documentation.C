/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

/** \file documentation.C
 * \brief container for doxygen documentation.
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
<li> \ref ExtentMap (see classes Extent and Extents)
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

Next: \ref ExtentMap

*/

/**  \page ExtentMap Extent and Extents objects

The core of the allocation system is a list of Extent objects.  Each
Extent contains a position and size.  In the list, the Extent objects
are ordered such that entry N's position plus entry N's size equals
entry N+1's position.  Cumulatively, the entire list thus describes
every position starting at zero at the start of the list thru
MAX_INT64, in order, with no gaps.

Each Extent can also be marked as used or free.  A used Extent
represents a region of a data file which is currently in-use.  A free
Extent represents a region which is unallocated.

\note It is meaningless to have two free Extent objects in a row.  If
such a condition occurs by freeing a used Extent, then the two
adjacent Extent objects should be coalesced into one.

When an allocation is required, an existing free Extent may be broken
into two Extent objects.  The size value of both Extent objects are
reduced (such that the sum still equals the original Extent size) and
the position of the second is increased (to maintain the consistency
of position plus size of the first one).

\section ExtentMapExample1 Allocation Example

Suppose prior to allocation, the Extents map looked like this:
<pre>
    1. Used 0-51 (size 52)
    2. Used 52-67 (size 16)
    3. Free 68-77 (size 10)
    4. Used 78-95 (size 18)
    5. Free 96-115 (size 20)
    6. Used 116-149 (size 34)
</pre>
Now we want to allocate 12.  If we look through the list, we see that
the hole at #3 is not large enough, but #5 is.  So we split it up.
<pre>
    1. Used 0-51 (size 52)
    2. Used 52-67 (size 16)
    3. Free 68-77 (size 10)
    4. Used 78-95 (size 18)
    5. Used 96-115 (size 12)
       Free 108-115 (size 8)
    6. Used 116-149 (size 34)
</pre>
So the new allocation of size 12 starts at position 96.

Now suppose instead we return to the original example, and decide we
want to free the extent of 18 at position 78.  After changing #4 from
the Used state to Free, we discover three free extents in a row.  So
we combine them as follows:
<pre>
    1. Used 0-51 (size 52)
    2. Used 52-67 (size 16)
    3. Free 68-115 (size 48)
    6. Used 116-149 (size 34)
</pre>

\section ExtentMapRemainder The Rest of the File

One special circumstance this code must manage is the final entry in
the Extents list.  If a file was of fixed maximum size, then the final
entry should extend out to this maximum value.  However, the disk file
should not have any limit.  (In practice of course, the operating
system limits the size of the file, typically to 2^63 bytes or
somesuch.)

Unfortunately, to save memory and disk space, the 'size' field of an
Extent is only an unsigned 31-bit value, since the size of a normal
extent should not be more than 2GB.  However, this 31-bit value is
insufficient to represent the concept of "From here to infinity".

The Extents object addresses this by using the maximum 31-bit value of
0x7FFFFFFF to represent infinity.  The size field of the final entry
in the Extents in-order list will never be modified during alloc or
free.

\section ExtentMapOptimize Optimizing Extent maps

This is well enough, but it needs to be efficient.  Obviously the
allocation case is not efficient, because we must walk the list until
a large enough hole is found.  (This is O(ln n)).  The free case is
also not efficient, because if an extent is identified by its starting
position, again you must perform an O(ln n) search to find the
matching extent.  If on the other hand you identify the extent by its
entry number in the table, the identifier changes each time some other
allocate or free action is taken elsewhere in the table.

The Extent and Extents objects solve the identification problem by
attaching a unique 32-bit identifer to each Used Extent.  This
identifier is stored in a hash table for quick lookup.  During an
alloc, the identifier is randomly generated (and the hash is checked
to ensure the same identifier doesn't already exist).  This
dramatically increases the speed of a free operation.

The allocation problem is solved as well, using a bucket-list array.
The free Extent objects are grouped on different linked lists by size.
In this implementation, the bucket size is 32 bytes.  Thus the first
bucket-list contains free holes of size 1-31 bytes in size.  The
second bucket-list contains free holes of size 32-63 bytes in size,
etc, on up to 65536 bytes in size.  This requires 2048 bucket-lists.

When an allocation is performed, the size of the desired allocation is
right-shifted by 5 bits and the resulting value is used to index the
array of buckets.  Thus a free extent of just the right size can be
located quickly.

However if there is no extent of just the right size, the bucket-list
array is walked in increasing order until another extent can be found.
This can be expensive since there are 2048 bucket-lists.  To optimize
this search, an array of 32 UINT64's are used as a bitmap.  An
algorithm races through the bitmap 64 bits at a jump looking for a bit
set to 1.  This corresponds to a bucket-list which is nonempty, which
can be used for allocation.  The first extent found is then broken
into two, and the remaining free portion is put back into the
bucket-list array in the position appropriate to its remaining size.

Next: \ref FileBlock

*/

/** \page FileBlock FileBlock object

 \section FileBlockExtents FileBlock Extents usage

The FileBlock controls allocation of the body of the file.  It uses an
Extents map to manage in-use and free zones in the file.  The Extents
is stored in a simple format on disk: the Extents is written an entire
page at a time.  Each entry is simply a pair of UINT32 values; the
first is an identifier key, while the second is the block size, with
the top bit reserved.  A value of 1 in the top bit indicates a block
which is in use, while 0 means a free region.  The remaining 31 bits
indicate the size of that particular block.  The file-offset of the
block is implied by summing all the previous extent-map descriptor
sizes.

Obviously, this implies that the on-disk format is not very useful for
actually managing allocations and frees.  When a file is first opened,
all of the extent-map pages are read in, and parsed.  A series of
lists are produced.  Every entry is on a linked list which is sorted
by the offset, in other words, in order through the file.  Every
entry, if a free region, is also on a bucket-list.  There are a
significant number of bucket lists, each corresponding to free regions
of different sizes.

The extent-map is stored on the disk in pieces.  The location of the
first piece is identified in the file-information block.

A flush-operation becomes rather expensive, because the entire linked
list of all extent-map entries must be serially written to the list of
extent-map file pages.  This is a disadvantage of this
technique.

\section FileBlockBuckets FileBlock Buckets

The free buckets are an array representing every multiple of 32 bytes
from 32 to 65536.  The buckets are arranged as an indexable array,
where the index is derived from a few bits of the piece size. Each
bucket is an in-memory btree (from dll2_btree.H) sorted by file
position.  This is done to maximize packing in the file.  Each time
that an allocation is performed, the caller is guaranteed to get the
lowest available piece of that size in the entire file.

Insertions into the bucket list are fast because of the
bitfield-indexing technique and speed of a memory-btree.  A
(significant) disadvantage of this technique is the memory
requirements.

\section FileBlockHash FileBlock Hash

Any block which is currently in-use, is also on a hash list.  The hash
key is the unique identifier.

One benefit of the unique identifier key method of retrieval, is that
the file can be compacted by moving entries near the end of the file
to unused holes earlier in the file.  But the block itself is still
retrieved using the same identifier, so that the application storing
data in the file is unaware the blocks have moved to a different
position.

A disadvantage of this techique is the size of the data structure
required to manage all the unique identifiers in the file.

\section FileBlockAccess FileBlock Access Methods

There are two separate phases in accessing a FileBlock file.  First there
is allocation management, in which there are three methods:
 FileBlockInterface::alloc, FileBlockInterface::realloc, and
FileBlockInterface::free.

These functions deal only in FileBlock ID numbers, and manipulating the 
Extents map.  An ID does not exist until an alloc call, and ceases to exist
after a free call.  When allocating an ID, the size must also be specified.
After the alloc call, there now exists a region within the disk file of the
specified size which is associated with this ID.

The realloc method is basically a free followed by another alloc of the new
size, except that the new allocation has the same ID number as the old one.
Note that this function does NOT copy the data from the old allocation to the
new one, the user is responsible for that.  (The reason is that the user may
wish to completely repopulate the new block, thus it is more efficient in that
case to let the user skip the copy step.)

The second phase is to actually access that region.  This is done with two
more methods, FileBlockInterface::get and FileBlockInterface::release.  The
first takes an ID number created by the alloc method, and returns a
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

Next: \ref FileBlockLocalFileFormat

*/

/** \page FileBlockLocalFileFormat FileBlockLocal File Format

\note All 4-byte or 8-byte numeric fields are encoded in big-endian format.

\section FileBlockLocalFileFormatFileHeader File Header

The first few bytes of the first page contains a unique signature
which identifies the file as a FileBlock file.  This signature must be
added when the file is first created.  The contents of the signature
is the ASCII string \em "FILE BLOCK LOCAL" with no trailing NUL (this
consumes 16 bytes).

The size for blocks of extents maps is flexible.  The recommended
value is 32KB; however they must not be more than 64KB in size.

The next 12 bytes contain an 8-byte offset and 4-byte size describing
a portion of the file containing a piece-map, described below.

\section FileBlockFileInfoBlock File Info Blocks

Following this are 64 4-byte units identifying block ids where file-
information blocks exist.  The value 0 means no information block is
populated in this slot.  A nonzero value indicates the presence of a
FileInfoBlockId structure.  This structure is 132 bytes in size.  The
first 4 bytes are a block id identifying the location of the user's 
info block.  The remaining 128 bytes are a character string uniquely
identifying the info block.

To enter a file block association into the file, the user specifies
the unique string and provides a FileBlock ID of the info block when
calling FileBlockInterface::set_data_info_block.

To retrieve this information later, the user specifies the unique string
and then calls FileBlockInterface::get_data_info_block.

If this information is no longer needed, the user should delete the 
association by calling FileBlockInterface::del_data_info_block.

\section FileBlockLocalFileFormatFileHeaderPieceMap Piece Map

A piece-map is a block in the file containing offset/size pairs.  Each
offset is an 8-byte value and each size is a 4-byte value.  Each
offset/size pair describes another block of the file containing a piece
of the extent map, described below.

\section FileBlockLocalFileFormatFileHeaderPiece Piece

The extent map entries are encoded 32KB at a time.  First the size of
an extent is encoded as a 4-byte big-endian number.  If the extent is
free, the highest-order bit is cleared.  If the extent is used, the
highest-order bit is set.  Also, if the extent is used, the size is
followed by a 4-byte encoding of the block id.  If the extent is free,
the block id is not present.

The file blocks where the the extent map is stored are themselves
encoded in the extent map.  This must be taken into account when the
extent map is being parsed to produce the file blocks.

\section FileBlockLocalFileFormatExample Example

A data file which has one million used extents and half a million free
extents will consume the following amount of disk space:

<pre>
1,000,000 x 8 bytes per extent = 8,000,000 bytes
  500,000 x 4 bytes per extent = 2,000,000 bytes
10,000,000 / 32768 bytes per piece = 306 pieces
</pre>

While running, it will also consume the following amount of RAM:

<pre>
1,500,000 extent map entries x
   (16 bytes per LListLinks * 4 LListLinks + 8 + 4 + 4) =
1,500,000 x 80 = 120,000,000 bytes
</pre>

(Note this does not count the overhead of the hash table.)

\see FileBlockHeader, PieceMapEntry

\section FileBlockLocalFileFormatFuture The future of the FileBlock interface

There is a lot of room for improvement in this API.  The Extents map is
expensive in terms of memory, disk, and time spent building the
on-disk image.  Unfortunately, the flexibility offered by the
position-independent identifier is required, thus Extents map cannot be
replaced by something like a free-space bitmap.

Of the expenses, the time spent reading and building the on-disk
representation of the Extents map is the most significant.  If a program
is designed to rarely flush, then this is less important (although the
startup time to initially open the data file is still important).
However if a long-running program wishes to frequently flush in order
to maintain consistency in the file, this time can become prohibitive
as the file size becomes large.

One possible optimization to the on-disk format might be to change the
format it is stored in.  Instead of making the extent offset an
implicit summation of all previous extent sizes, the offset could be
explicitly included in each on-disk Extent.  Thus the in-order
restriction could be relaxed.  Also, if the unique identifier was
present even for free-blocks (even though it is not used), then all
Extent entries are fixed-size on disk.  This would present the
opportunity for a free-space bitmap of the Extent "table" allowing
individual slots in the table to be freed and allocated as Extent
splits and joins occur.  This would dramatically reduce the time
required to flush recent changes back to the disk-- only those entries
in the table modified since the last flush need be written.  The
disadvantages of this technique are that it increases both the
complexity and footprint of the on-disk representation.  This
technique also does not address the initial load-time problem nor does
it address the memory footprint of the Extents map.

The problem with addressing the memory footprint of the Extents map is
the unique identifier translation.  In order to efficiently translate
an identifier into a file position, it is currently mandatory for the
entire translation table (i.e. the Extents map) to be memory resident,
thus also mandating loading the entire table at file-open time.  Any
solution which wishes to solve the memory-footprint issue must find a
way to address the unique identifier problem while still maintaining
decent lookup speed.

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
     <li> UINT32_t ptr
     	  <ul>
	  <li> this is a FileBlock ID number of a pointer to a child node.
	  </ul>
     <li> UINT16_t keystart
     <li> UINT16_t keysize
     	  <ul>
	  <li> these indicate where the key data for this item begin within
	       the keydata element below.
     	  </ul>
     <li> UINT32_t data
     	  <ul>
	  <li> this is a FileBlock ID number of the data corresponding to
	       the key above.
	  </ul>
     </ul>
<li> UINT32_t ptr
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
                                     UINT32 blockid,
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
                                     UINT32 blockid,
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
