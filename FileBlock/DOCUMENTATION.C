 
/** \file DOCUMENTATION.C
 * \brief Container file for the Doxygen documentation
 * \author Phillip F Knaack
 *
 * You will find nothing in this file except Doxygen documentation. */
// You will find nothing in this file except Doxygen documentation.

/** \mainpage FileBlock Interface

The purpose of the FileBlock interface is to manage allocation of file
space within a file.  An excellent analogy to the FileBlockInterface 
object are the standard unix functions \c malloc and \c free, except that
it is for file space rather than memory space.

 \section FileBlockObjects FileBlock component objects

The FileBlock interface is composed of a number of lower-level object
types.  Click on the following links to read about each of them.

<ul>
<li> \ref PageIO
<li> \ref PageCache
<li> \ref BlockCache
<li> \ref ExtentMap
<li> \ref FileBlock
</ul>

 \page PageIO PageIO object

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

 \page PageCache PageCache object

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

 \page BlockCache BlockCache object

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

 \page ExtentMap Extent and Extents objects

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

 \page FileBlock FileBlock object

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
entry, if a free region, is also on a bucket-list.  There a
significant number of bucket lists, each corresponding to free regions
of different sizes.

The extent-map is stored on the disk starting with the first page.
The first few bytes of the first page contains a unique signature
which identifies the file as a FileBlock file.  This signature must be
added when the file is first created.

\todo needs more information here.

A flush-operation becomes rather expensive, because the entire linked
list of all extent-map entries must be serially written to the list of
extent-map file pages.  This is the only disadvantage of this technique.

\section FileBlockBuckets FileBlock Buckets

The list of buckets is configurable by the user when the FileBlock
object is created.  The reason, is that the bucket-list which is
useful varies greatly by application.  If a given application uses
only 3 different block sizes, then a small but specific bucket-list
may be appropriate for speed.  On the other hand if bucket sizes vary
greatly by a large range, the application may define a short list of
ranges comprising the most common bucket sizes.  Or, if it is
important to optimize the size of the file as much as possible, it may
be desirable to define a much larger bucket-list with small increments
between them.  This will be slower to search for appropriate sizes,
but will dramatically improve file space utilization.

Another option which needs investigation, is to arrange the
bucket-list as an indexable array, where the index is derived from a
few bits of the piece size.

\section FileBlockHash FileBlock Hash

Any block which is currently in-use, is also on a hash list.  The hash
key is the unique identifier.

One benefit of the unique identifier key method of retrieval, is that
the file can be compacted by moving entries near the end of the file
to unused holes earlier in the file.  But the block itself is still
retrieved using the same identifier, so that the application storing
data in the file is unaware the blocks have moved to a different
position.

\section FileBlockAccess FileBlock Access Methods

There is a single allocation function which supports all the features
required in the allocator; all other allocation functions are merely
front-ends to this one function.

\todo needs more information here.

 */
