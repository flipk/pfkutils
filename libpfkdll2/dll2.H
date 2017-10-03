
/** \file dll2.H
 \brief doubly-linked lists
 \author Phillip F Knaack

 \mainpage Doubly-Linked Lists (version 2)

This is a general purpose library for robust, safe, self-validating, and
easy-to-use doubly linked lists of any data type.

 \section dll2copyright Copyright notice

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

 \section dll2features Features

The DLL2 library includes the following features:

<ul>
<li>   LListHash : self-balancing hash tables that support millions
       of elements and user-defined comparators for fast hash search.
  <ul>
  <li>   Insert or remove at head or tail.
  </ul>
<li>   LListOrderedQ : ordered queue (i.e. for fast timer lists).
<li>   LListLRU : least-recently used lists with promote functionality.
<li>   LListHashLRU : compound container type demonstration
<li>   LListBTREE : memory btree of arbitrary order.
<li>   LListLinks : list links placed into each item
<li>   linked list checksums and validations.
<li>   crosslinked list detection and prevention.
<li>   uninitialized object prevention.
<li>   in-use object deletion prevention.
</ul>

 \section dll2contents  Contents

\ref dll2discussion

\ref dll2usage

\ref dll2config

 \page dll2discussion DLL2 Discussion

 \section Introduction

Linked lists are an integral part of complex software designs.  Many
processes inevitably require linked lists containing hundreds or
thousands of items; complex access methods are also required, such as
hash tables, least-recently-used lists, and the ability to place an
item on two or more lists simultaneously.

History has taught us that outside libraries for managing linked lists
are often difficult to use and/or limited in some important way that
makes the library undesirable.  Thus, applications typically home-grow
their own linked-list implementation, either through a library of
functions, or by repetition of code in each module that references the
list.  When this is done, history also teaches us that such
implementations frequently result in unstable code that experiences
corrupted linked lists – and often in such a way that is not
detectable until a significant amount of time has passed.  Analyzing
bus faults and other debug data from such problems rarely leads to a
root cause in a trivial way.

The DLL2 API attempts to abstract many of these operations, providing
the flexibility needed for complex structures while remaining simple
to use, and also providing safety nets to cause coding errors with
linked lists to be evident much sooner in the development lifecycle –
as well as helping ensure that information gathered from fault reports
will conclusively identify the location of the root cause directly,
rather than indirectly or not at all.

 \section dll2errorscaught Programming errors caught by this API

When discussing the design decisions for the DLL2 API, it is important
to first discuss the types of programming errors the library is
designed to catch.  These are programming errors which have been
discovered through years of experience of debugging problems on
applications currently running in the field.

The following is a list of the types of programming errors that this
API will catch on their first occurrence, unlike with other linked
list libraries that may allow such errors to corrupt lists and cause
software errors much later.

- Freeing an item while still on a list

It is a common error for a module to free an item back to the free
memory pool, even though that item still has pointers pointing to it
from the next and previous members of another object (or the head/tail
object).  This buffer may be reused at some point in the future, which
causes the linked list to become corrupt or crosslinked.  For
instance, if the buffer was reallocated as the same data type and then
inserted on another list, and some other module attempted to walk the
first list from start to finish, it would walk the list up to the
point where it points to this buffer, and then jump to the other list
without realizing it.  Many behaviors may result from this operation,
and it is very difficult to debug.

- Adding item to list when already on another list

It is also a common error for a module to reference an item and begin
the operations to put it on a list, not realizing it was already on
some other list (or even the same list already!).  This causes
pointers to existing items to be overwritten within the object.
Sometimes this results in loosing items forever off the end of the
list (a memory leak).  Other times this causes crosslinks from one
list to another.  An even worse side effect, if the item is being
added to a list it is already on, it can cause a circular list, which
eventually sometime later throws the process into an infinite loop!

- Adding new item before/after existing item when wrong list referenced

This is a non-deterministic situation with many possible scenarios.
One scenario is that the operation works normally when the referenced
items exist somewhere in the middle of the list, but fails if one of
the items is at the end of the list.  This is because linked list
algorithms must take special care when dealing with the endpoints of a
list to update the head or tail pointers of the list.  When the module
is confused about which list the items are on, it may update the
head/tail pointers of the wrong list, causing a crosslink or circular
list.

- Removing item from a list it isn’t actually on

Like the previous item, this one can also cause the wrong head/tail
pointers to be updated, causing crosslinks and circular lists.

- Walking a list when an item isn’t on the list we think it is

This one is not so obvious.  Suppose a module has a for-loop that is
walking from the head of a list to the tail; suppose the next-item
clause (the third clause) of the for-statement fetches the ‘next’
pointer.  This is a very natural thing to do.

\code
for ( item = list.get_head();
      item != NULL;
      item = list.get_next(item) )
{
    if ( […item should be moved…] )
    {
        list.remove(item);
        other_list.add(item);
    }
}
\endcode

But, suppose also that somewhere within the for-loop, a particular
statement decides the current item needs to be removed from this list
and placed on another list.  Once the code returns to the top of the
for-loop to follow the ‘next’ pointer, it follows a next pointer that
now belongs to a different list. This for-loop is now following items
on the wrong list.  Such a bug can take weeks to notice and longer to
debug.  The DLL2 API will catch this error on the very first
occurrence and report it—specifically, the list.get_next method will
report that the item is not on this list.

A corrected version of this code appears here:

\code
for ( item = list.get_head();
      item != NULL;
      item = next_item )
{
    next_item = list.get_next(item);
    if ( […item should be moved…] )
    {
        list.remove(item);
        other_list.add(item);
    }
}
\endcode

- Following a ‘next’ pointer when an item has been deleted

Another situation is one where the current item is deleted from the
list and freed. In this case, the code is going to follow a ‘next’
pointer that resides in free memory.  On many platforms, this may
produce a race condition in which another process or thread may
reallocate that piece of memory for another purpose and change it
before our process has a chance to read the ‘next’ pointer.  In
addition, on many demand-paged UNIX systems, the free-operation may
cause dynamically mapped memory to be unmapped from the process
address space – so this code will produce a bus fault that will only
occur occasionally and randomly.

- The time delay

The most difficult aspect of these problems is the time difference
between the user-visible symptom and the actual operation that caused
the problem.  It has often occurred in the past that a linked list
corruption occurred minutes or even hours before a bus fault or
infinite loop lockup occurred.  Root cause diagnosis is very difficult
in these cases, because the bus fault or errant behavior can provide
no clues to the root cause.  The bus fault will confirm that the
linked list is corrupt, but there is no way to know what sequence of
operations corrupted it.  So it is important to catch these types of
bugs much earlier, preferably during unit test rather than in the
field.  Most of the design decisions made in the DLL2 API are towards
the goal of discovering coding errors during unit test.

 \section dll2addressing How does DLL2 address these issues?

- Items know what list they are on

The most important feature of the DLL2 API is this: not only do the
lists point to the items, but the items also point to the lists.  Each
LListLinks object has three pointers in it: a ‘next’ pointer, a ‘prev’
pointer, and an ‘onlist’ pointer.  (A debugging version of the DLL2
library also contains a fourth member, a ‘checksum’ over the other
three.  This is to catch those cases where rogue applications access
these fields accidentally (due to out-of-bounds pointer or similar
idea)). This ‘onlist’ pointer is NULL whenever the item is not on a
list.  When the item is on a list, the ‘onlist’ pointer points to the
head/tail object.  This allows the following validations to take
place:

When an item is added to a list, we can ensure that it was not
previously on any list by verifying that ‘onlist’ is currently NULL.

When an item is removed from a list, we can verify it really is on the
list we are trying to remove it from.

When an item’s memory is freed (via ‘delete’), we can verify that the
item has been removed from all relevant lists prior to deletion by
ensuring that all ‘onlist’ pointers are NULL.

When following a ‘next’ pointer to walk a list, we can verify that the
item really is on the list we think it is on.

- Automatic constructors / destructors

Coding the DLL2 API in C++ gives options not available in C.  For
instance, the LListLinks structure has a constructor which
automatically initializes the next/prev/onlist pointers to NULL,
guaranteeing that the item’s list links start out in an initialized
state and the item is marked initially as not being on any list.
Immediately the error found in is addressed, and is partially
addressed.  Also, destructors validate that ‘onlist’ is NULL, meaning
that the item is no longer on a list.

- Fully updating pointers

Many linked-list implementations do not update the pointers within an
object when the object is removed from a list.  Thus, errant code may
later try to continue walking a list when an item is no longer on that
list, causing the code to jump into the middle of a list or into the
middle of some other list.  All of the DLL2 API ‘remove’ operations
fully update all of the pointers, i.e. ‘next’, ‘prev’ and ‘onlist’ are
all written to NULL when the item is removed from the list.

 \section dll2conclusion Conclusion

Many of the operations described in this document may seem like extra
work, cutting into the efficiency of a program, like validating
‘onlist’ against the list head on every iteration of a for-loop,
writing ‘next’ and ‘prev’ to NULL while removing from a list even
though the item is just about to be freed anyway, writing ‘onlist’ to
NULL even though an item is about to be added to another list, and
others.

In truth, a program which is fully debugged and stable can do without
these extra operations, in fact in a future version of the DLL2
library, the author may consider adding a compile-time option which
removes many of the validations.  However, it takes a long time and a
lot of work to get a large program into product-quality where these
extra operations are no longer necessary, and these operations are
worth their extra CPU time when you consider how much time they save
in debugging problems when they arise.

Finally, in the author’s opinion, no software exists in the world
which is “fully debugged and stable.”


 \page dll2usage Usage

The entire DLL2 C++ API is provided by a single header file, dll2.H.
This header file contains a series of C++ template definitions for the
following objects:

- LListLinks: a data type added to each data item to provide list linkage
- LList: a basic doubly linked list with add and remove operations
- LListHash: a hash table implementation with add, remove, and find operations
- LListLRU: a least-recently-used list implementation with add,
  remove, promote, and get_oldest operations
- LListOrderedQ: an ordered queue linked list (useful for e.g. a timer list)
- LListHashLRU: an example implementation of a compound container
  class which stores items on a hash table and a least-recently-used
  list simultaneously (useful for e.g. a disk cache)

The DLL2 API allows an item to be on more than one list
simultaneously.  It does this by allowing the LListLinks type to be
defined as an array within an item.  Each list is then told which
entry in the array to use when following the pointers relevant to that
list.

The figure below shows an example of a LListHashLRU and its component
LListHash and LListLRU structures.  All list pointers relevant to the
LListHash are shown in red and all pointers relevant to the LRU are
shown in green.  The resulting lists may be followed from the “head”
pointer and through the “next” pointers until NULL is reached, or they
may be followed starting at a “tail” pointer and through the “prev”
pointers until NULL is reached.

\image html llisthashlru.jpg

Note also that the LListHashLRU does not allow access to its component
LListHash or LListLRU members directly; the only access is through a
set of its own access methods, which operate on both the component
objects simultaneously.  This guarantees that the items on the list
cannot become internally inconsistent (e.g. it would not be possible
to have an item on the LListHash but not on the LListLRU).

 \section dll2listinstancenumber List instance number

The DLL2 API introduces a concept called a “list instance number.”  So
that an item may exist on more than one list simultaneously, the item
must contain an array of “link” objects containing “next” and “prev”
pointers.  The index of this array corresponds to the list instance
number.

When designing a data structure involving multiple linked lists, it is
helpful to consider the instance number so that a maximum of
flexibility may be provided while minimizing the amount of space
required in each item.

For example, suppose an item of some type has three lists it may exist
on:
- an active list;
- a standby list; and
- a hash table

hashed by identification number.  Initially this would suggest that
three instance numbers are required; however upon further
consideration of the algorithms which operate on these structures, we
may realize that an item cannot be active and standby at the same time-–thus
at any given instant the item cannot be on both the active and
standby lists.  Thus, the active and standby list may both use
instance number zero.  We may also determine that an item will always
be simultaneously on the ID hash, thus the ID hash may use instance
one.  Each item then requires an array of only two list link
structures.

It is also helpful to define an “enum” type to identify the list
instance numbers.  In the previous example, the enums could appear as
in the below snippet.

\code
enum {
    ACTIVE_STANDBY_LIST,  // instance 0
    HASH_LIST,            // instance 1
    NUM_LISTS
};
\endcode

As another example, suppose we are managing a disk cache, with one
item per disk block in the cache.  We require a hash table to locate
each disk block given only the block number, as well as a
least-recently-used list so that old items may be expired from the
cache.  Suppose also that we want a linked list of all the “dirty”
disk blocks (blocks modified in memory but not yet written to disk)
and a linked list of all “clean” blocks (whose buffer contents match
the disk contents).  Since a block cannot be both clean and dirty at
the same time, these two lists share a list instance number.  The
“enum” might appear as follows:

\code
enum {
   DISK_CACHE_HASH_LIST,        // instance 0
   DISK_CACHE_LRU_LIST,         // instance 1
   DISK_CACHE_CLEAN_DIRTY_LIST, // instance 2
   DISK_CACHE_NUM_LISTS
};
\endcode

In each of these enum definitions, the final enum (NUM_LISTS or
DISK_CACHE_NUM_LISTS above) is handy when declaring the links array in
the item definition, since it ensures that if additional instances are
added in the future, the links array will automatically grow to
accommodate the new list.  See the definition of struct disk_block in
the snippet below for an example of this.

 \section dll2llistlinks LListLinks definition

When declaring the data type that will be stored in a linked list, the
data type must itself contain the “links” pointers used by the linked
lists.  The name of this member must be “links.”  It must also be an
array – even if the data type exists on only one list, in which case
the array size is one.

The LListLinks data type is actually a C++ template, and the template
invocation (as noted by the angle brackets < > in the following
figure) has one argument: the name of the data type itself.  When
declaring the data structure for the disk block example described
above, it might appear as in this snippet:

\code
struct disk_block {
   LListLinks<disk_block>  links[ DISK_CACHE_NUM_LISTS ];
   [… other structure members appear here …]
};
\endcode

The user of the DLL2 API must never reference the contents of the
LListLinks object.  The members are marked as private, just to ensure
that this cannot be done without some effort.  To do so would bypass
the protection mechanisms.

\note  One of the implications of this is that the user of this API
cannot follow ‘next’ or ‘prev’ pointers directly from an item.  The
user must reference ‘next’ and ‘prev’ pointers through the ‘get_next’
and ‘get_prev’ methods found in the list objects.  This is to force
validations at each step.

 \section dll2LList LList object

In order to maintain a list of objects, the list object itself must be
declared.  When declaring a list object, two things must be specified:
the name of the data type which will be stored in the list, and the
instance number that list should use when referencing the links array
within that data type.  For instance, to declare the LRU list object
for the disk cache described above, the declaration may appear as
follows.

\code
LListLRU<disk_block,DISK_CACHE_LRU_LIST>  disk_cache_lru;
\endcode

Note that it may also be convenient to declare a new data type for a
list in which the instance number is already encoded into the type.
The following two sets of list declarations for the disk cache clean
and dirty lists are exactly equivalent.

\code
LList<disk_block,DISK_CACHE_CLEAN_DIRTY_LIST> disk_cache_clean_list;
LList<disk_block,DISK_CACHE_CLEAN_DIRTY_LIST> disk_cache_dirty_list;

typedef LList<disk_block,DISK_CACHE_CLEAN_DIRTY_LIST> Disk_cache_clean_dirty_list_t;
Disk_cache_clean_dirty_list_t  disk_cache_clean_list;
Disk_cache_clean_dirty_list_t  disk_cache_dirty_list;
\endcode

Refer to the documentation for the LList class for a list of methods
it provides.

 \section dll2listhash LListHash object

In order to use the LListHash data type, the user must implement a
‘comparator’ class.  This is a method-only class (no data) which
implements helper functions to aid in the calculation of optimal hash
indexes.  A template appears below.

\code
class ComparatorClassName {
public:
    static int hash_key( ObjectType * b ) {
        // perform a manipulation on b
        // to return a hashable integer
    }
    static int hash_key( const HashType id ) {
        // perform similar manipulation on
        // id to return hashable integer
    }
    static bool hash_key_compare( ObjectType * b,
                                  const HashType id ) {
        // return ‘true’ or ‘false’
        // based on ‘b’ and ‘id’
    }
};
\endcode

When declaring a LListHash object, the following things must be
specified: the data type of the object being stored (ObjectType), the
data type of the identifier used for the hashing function (HashType),
and the name of the comparator class used (ComparatorClassName).  By
specifying these in the LListHash declaration, the user is given the
flexibility to use any data type for indexing the hash.  For example,
a disk cache may wish to have a hash based on block number (specified
as type ‘int’), while a pre-compiler may wish to have a hash based on
the name of a macro (specified as type ‘char *’).

Any hash, in the end, requires an integer to determine an index into
an array.  The ‘hash_key’ methods of the comparator class should
return any valid positive integer.  Internally, the LListHash object
will modulo this value with the dimension of the array to arrive at a
hash position; however the comparator class should not attempt to take
any hash dimension into account.  The LListHash object automatically
resizes the array based on the number of elements to optimize the
search time.  The only requirement on the comparator hash_key methods
is that for a given element, they always return the same value every
time.  It is also acceptible for two different objects to return the
same hash_key value—this is the reason for the hash_key_compare
method.  Once an array index has been calculated, an extra comparison
is performed using hash_key_compare to locate the proper item.

This syntax allows an object to be present on multiple hash lists
simultaneously, keyed off of different datums within the object.  For
example, a group of file descriptors may be simultaneously hashed
based on the file descriptor number (where HashType is ‘int’) as well
as the name of the file (where HashType is ‘char *’).  The user must
simply define two comparator classes, each using the relevant HashType
and referencing the appropriate members of the object.

Refer to the LListHash documentation for a list of methods it provides.

 \section dll2llistlru LListLRU

This object implements a least-recently-used list. Internally it is
implemented as a LList.  The tail of this list represents the newest
item and the head is the oldest.

 \section dll2listorderedq LListOrderedQ

The ordered queue is a curious data structure with its origins in the
UNIX operating system for managing timer lists.  The item data type
must provide an integer called ordered_queue_key for internal use by
the OrderedQ.  When an item is placed on the list, an ordered key
value is specified for the item.  The list is maintained in order of
this ordered queue, with smaller values first and larger values later.
The list is walked in order looking for an appropriate location to
insert the new item and maintain the ordered queue order.  An extra
twist is that the ordered_queue_key value is relative to the item
preceding it.  This is appropriate for a list of timers, because on
each clock tick, only the item at the head of the list must be
examined.  The ordered_queue_key value for that item represents the
number of ticks until that item expires.  The key value is decremented
on each clock tick.  Once the key value reaches zero, the item is
removed and processed, exposing the next item on the list – and
because the key values are relative, the key value in the exposed item
again represents the number of ticks until it expires.  Because of
this arrangement, timer management is quite efficient: the only
operation which is not O(1) is the insert operation, which is O(log
n).

An example appears in the following figures.  Suppose three timers are set:
- (a) expires in 12 seconds; 
- (b) expires in 15 seconds;
- (c) expires in 20 seconds.

The list would appear in this case as shown.  As each second ticks by,
the key is decremented for the head item.  The second figure below
shows the configuration 9 seconds later.  Three seconds after that,
timer (a) key value reaches zero, and expires and is removed, exposing
timer (b) at the head of the list with a key value of 3.  The third
figure shows the configuration one tick after that.

\image html figure8.jpg
\image html figure9.jpg
\image html figure10.jpg

Refer to the documentation for LListHashLRU for methods it provides.

 \section dll2customcompoundcontainer DLL2 Compound Container Types

If data items must exist on multiple lists at one time, it may make
more sense to implement a single class which contains those multiple
lists and provides a set of simple methods to manipulate them, in
order to ensure that the state of the lists remain consistent between
lists.  For example, in our disk cache example, it would be invalid
for a disk_block to be on the identification hash but not on either
the clean or dirty block list.  It would also be invalid if the
disk_block were not simultaneously on the LRU.

A compound container class can provide a simple set of operations that
manipulate the individual lists in such a way to maintain consistency
across all relevant lists.  Two examples are provided.  The DLL2 API
header file contains a class called LListHashLRU, which is described
below.  Also described is a container class for the disk cache example
described throughout this chapter.

- LListHashLRU compound container example

\code
template <class T, int hash_instance, int lru_instance>
class LListHashLRU {
private:
    LListHash <T, hash_instance> hash;
    LListLRU  <T, lru_instance > lru;
public:
    T * find( int key ) { return hash.find( key ); }
    int get_cnt( void ) { return lru.get_cnt(); }
    void add( T * t ) { hash.add( t ); lru.add( t ); }
    void remove( T * t ) { hash.remove( t ); lru.remove( t ); }
    void promote( T * t ) { lru.promote( t ); }
    bool onlist( T * t ) { return lru.onlist( t ); }
    bool onthislist( T * t ) { return lru.onthislist( t ); }
    T * dequeue_lru_head( void ) {
        T * ret = lru.get_head();
        if ( ret ) remove( ret );
        return ret;
    }
    T * dequeue_lru_tail( void ) {
        T * ret = lru.get_tail();
        if ( ret ) remove( ret );
        return ret;
    }
    T * get_lru_head ( void ) { return lru.get_head(); }
    T * get_lru_tail ( void ) { return lru.get_tail(); }
    T * get_lru_next( T * t ) { return lru.get_next(t); }
    T * get_lru_prev( T * t ) { return lru.get_prev(t); }
};
\endcode

Since the LListHashLRU contains both a hash and LRU, it requires two
sets of LListLinks to implement.  Thus when the LListHashLRU template
is instantiated, two instance numbers must be specified, and the
declaration of the item data type must include enough LListLinks
instances to cover them as well as whatever lists the items may be
used with.

Note that the hash and LRU data members are private.  The only access
to them is through the member functions, which modify both the hash
and LRU simultaneously.  This is to ensure that their states remain
consistent relative to each other (i.e. it would be impossible to have
an item on the LRU but not on the hash).

This example also provides methods sufficient for walking the LRU in
either order and for promoting the item in the LRU.

- Disk Cache compound container example

\code
class Disk_Cache {
private:
   int max_cache_size;
   LListHash<disk_block,DISK_CACHE_HASH_LIST>  hash;
   LListLRU<disk_block,DISK_CACHE_LRU_LIST>    lru;
   Disk_cache_clean_dirty_list_t               clean;
   Disk_cache_clean_dirty_list_t               dirty;
   void remove( disk_block * b ) {
      if ( dirty.onthislist( b ))
      {
         b->write_block();
         dirty.remove( b );
      }       
      else
      {
         clean.remove( b );
      }
      lru.remove( b );
      hash.remove( b );
   }       
public:
   Disk_Cache( int hash_size, int _max_cache )
      : hash( hash_size ) {
         max_cache_size = _max_cache;
   }
   ~Disk_Cache( void ) { [......] }
   disk_block * get( int block ) {
      disk_block * ret = hash.find( block );
      if ( ret != NULL )
      {
         lru.promote( ret );
         return ret;
      }
      while ( lru.get_cnt() >= max_cache_size )
      {       
         ret = lru.get_oldest();
         remove( ret );
         delete ret;
      }
      ret = new disk_block( block );
      lru.add( ret );
      hash.add( ret );
      clean.add( ret );
      return ret;
   }
   void mark_dirty( disk_block * b ) {
      if ( dirty.onthislist( b ))
          return;  // already dirty
      clean.remove( ret );
      dirty.add( ret );
   }       
   void flush( void ) {
      disk_block * b, * next_b;
      while ( b = dirty.dequeue_head() )
      {
         b->write_block();
         clean.add( b );
      }
   }
};
\endcode

*/

/** \todo read http://www.stack.nl/~dimitri/doxygen/external.html */

#ifndef __DLL2_H_
#define __DLL2_H_

/** \defgroup dll2config DLL2 Configurable Items */
/** \defgroup dll2listtypes DLL2 list types */

#ifndef DLL2_CHECKSUMS
/** enable use of checksum in linked list protection
 * \ingroup dll2config
 */
#define DLL2_CHECKSUMS      1
#endif

#ifndef DLL2_INCLUDE_BTREE
/** enable inclusion of btree definition
 * \ingroup dll2config
 */
#define DLL2_INCLUDE_BTREE  0
#endif

#ifndef DLL2_STRICT_WALKING
/** enable strict checking of linked list pointers
 * \ingroup dll2config
 */
#define DLL2_STRICT_WALKING 1
#endif

#ifndef DLL2BAIL
#include <stdio.h>
/** how to handle a validation failure
 * redefine this macro before including dll2.H to define 
 * your own method for handling a validation failure.
 * \ingroup dll2config
 */
#define DLL2BAIL(reason) { printf reason ; kill( 0, 6 ); }
#endif

/* config items end here */

#include <sys/types.h>
#include <signal.h>

template <class T, int instance> class LList;

/** link pointers placed in any item which is to live on 1 or more lists.
 * include in your data type like so:
 *       \code LListLinks<T> links[ (number of lists) ]; \endcode
 * \ingroup dll2listtypes
 * \param T the type of the item this link is stored in.
 */
template <class T>
class LListLinks
{
private:
    friend class LList<T,0>;  // hack! hack! hack! --bill the cat
    friend class LList<T,1>;
    friend class LList<T,2>;
    friend class LList<T,3>;
    T * next;
    T * prev;
    void * onlist;
#if DLL2_CHECKSUMS
    unsigned long checksum;
    unsigned long calc_checksum(void) const {
        return 0xb09d3086U + (unsigned long) next +
            (unsigned long) prev + (unsigned long) onlist;
    }
    void validate(void) const {
        if ( checksum != calc_checksum())
            DLL2BAIL(( "CHECKSUM ERROR IN DLL2 ELEMENT\n" ));
    }
    void recalc(void) { checksum = calc_checksum(); }
#else
    void validate(void) const {}
    void recalc(void) {}
#endif
public:
    /** constructor, initializes links and checksum */
    LListLinks( void ) {
        next = prev = NULL;
        onlist = 0;
        recalc();
    }
    /** destructor, validates item is no longer on any list */
    ~LListLinks( void ) {
        if ( onlist != 0 )
            DLL2BAIL(( "ERROR LLIST ITEM DELETED BUT STILL ON A LIST!\n" ));
#if DLL2_CHECKSUMS
        checksum = 0;
#endif
    }
    /** retrieves pointer to LList object this item is on */
    void * get_onlist( void ) const { return onlist; }
};

/** a simple doubly-linked list
 * \ingroup dll2listtypes
 * \note In order to use LList, type T must have the following member: 
 *       \code LListLinks<T> links[ (number of lists) ]; \endcode
 * \param T the type of the items to store in the list
 * \param instance the list instance number, basically the LListLinks index
 */
template <class T, int instance>
class LList
{
    T * head;
    T * tail;
    int cnt;
#if DLL2_CHECKSUMS
    unsigned long checksum;
    unsigned long calc_checksum(void) const {
        return 0x2df692f8U + (unsigned long) head +
            (unsigned long) tail + (unsigned long) cnt;
    }
    void validate(void) const {
        if ( checksum != calc_checksum())
            DLL2BAIL(( "CHECKSUM ERROR IN DLL2 LLIST\n" ));
    }
    void recalc(void) { checksum = calc_checksum(); }
#else
    void validate(void) const { }
    void recalc(void) const { }
#endif
public:
    /** constructor, initializes head, tail, and checksum */
    LList( void ) { head = tail = NULL;  cnt = 0; recalc(); }
    /** destructor, validates list is empty, error if not */
    ~LList( void ) {
        validate();
        if ( head || tail || cnt )
            DLL2BAIL(( "LLIST DELETED BUT NOT EMPTY\n" ));
#if DLL2_CHECKSUMS
        checksum = 0;
#endif
    }
    /** add an item of type T to tail of list */
    void add( T * x ) {
        validate();
        LListLinks<T> * ll = & x->links[instance];
        ll->validate();
        if ( onlist( x )) 
            DLL2BAIL(( "ERROR LLIST ENTRY ALREADY ON LIST\n" ));
        ll->onlist = (void*)this;
        ll->next = NULL;
        ll->prev = tail;
        if ( tail )
        {
            tail->links[instance].next = x;
            tail->links[instance].recalc();
        }
        else
            head = x;
        ll->recalc();
        tail = x;
        cnt++;
        recalc();
    }
    /** remove an item of type T from the list, no matter
     * it's position in the list */
    void remove( T * x ) {
        validate();
        LListLinks<T> * ll = & x->links[instance];
        ll->validate();
        if ( !onthislist( x ))
            DLL2BAIL(( "ERROR LLIST ENTRY NOT ON THIS LIST\n" ));
        ll->onlist = 0;
        if ( ll->next )
        {
            ll->next->links[instance].prev = ll->prev;
            ll->next->links[instance].recalc();
        }
        else
            tail = ll->prev;
        if ( ll->prev )
        {
            ll->prev->links[instance].next = ll->next;
            ll->prev->links[instance].recalc();
        }
        else
            head = ll->next;
        ll->next = ll->prev = NULL;
        ll->recalc();
        cnt--;
        recalc();
    }
    /** add an item after an existing item (closer to tail) */
    void add_after( T * item, T * existing ) {
        validate();
        LListLinks<T> * exll = & existing->links[instance];
        LListLinks<T> * itll = & item->links[instance];
        exll->validate();
        itll->validate();
        if ( !onthislist( existing ))
            DLL2BAIL(( "ERROR CANNOT DO add_after WHEN EXISTING ITEM "
                   "NOT ON THIS LIST\n" ));
        if ( onlist( item ))
            DLL2BAIL(( "ERROR CANNOT DO add_after WHEN NEW ITEM "
                   "IS ALREADY ON A LIST\n" ));
        itll->onlist = (void*)this;
        itll->next = exll->next;
        itll->prev = existing;
        exll->next = item;
        if ( itll->next )
        {
            itll->next->links[instance].prev = item;
            itll->next->links[instance].recalc();
        }
        else
            tail = item;
        exll->recalc();
        itll->recalc();
        cnt++;
        recalc();
    }
    /** add an item before an existing item (closer to head) */
    void add_before( T * item, T * existing ) {
        validate();
        LListLinks<T> * exll = & existing->links[instance];
        LListLinks<T> * itll = & item->links[instance];
        exll->validate();
        itll->validate();
        if ( !onthislist( existing ))
            DLL2BAIL(( "ERROR CANNOT DO add_before WHEN EXISTING ITEM "
                   "NOT ON THIS LIST\n" ));
        if ( onlist( item ))
            DLL2BAIL(( "ERROR CANNOT DO add_before WHEN NEW ITEM "
                   "IS ALREADY ON A LIST\n" ));
        itll->onlist = (void*)this;
        itll->prev = exll->prev;
        itll->next = existing;
        exll->prev = item;
        if ( itll->prev )
        {
            itll->prev->links[instance].next = item;
            itll->prev->links[instance].recalc();
        }
        else
            head = item;
        exll->recalc();
        itll->recalc();
        cnt++;
        recalc();
    }
    /** return true if the named item is currently on any list of
     * this instance number */
    bool onlist ( const T * t ) const {
        return t->links[instance].onlist != 0;
    }
    /** return true if the named item is currently on this list */
    bool onthislist ( const T * t ) const {
        return t->links[instance].onlist == (void*)this; 
    }
    /** return the number of items currently on this list */
    int get_cnt  ( void ) const {
        return cnt;
    }
    /** remove the head item from the list and return it */
    T * dequeue_head( void ) {
        T * ret = head;
        if ( ret ) remove( ret );
        return ret;
    }
    /** remove the tail item from this list and return it */
    T * dequeue_tail( void ) {
        T * ret = tail;
        if ( ret ) remove( ret );
        return ret;
    }
    /** return the item that is at the head of the list but don't
     * change the list */
    T * get_head ( void ) const {
        validate();
        return head;
    }
    /** return the item that is at the tail of the list but don't
     * change the list */
    T * get_tail ( void ) const {
        validate();
        return tail;
    }
    /** given an existing item which is currently on the list,
     * return the item which follows it (closer to tail).
     * \note It is an error to supply an existing item that is not
     *       not currently on this list. */
    T * get_next( T * x ) const {
        validate();
#if DLL2_STRICT_WALKING
        if ( !onthislist( x ))
            DLL2BAIL(( "ERROR get_next ITEM is not on this list!\n" ));
#endif
        x->links[instance].validate();
        return x->links[instance].next;
    }
    /** given an existing item which is currently on the list,
     * return the item which precedes it (closer to head).
     * \note It is an error to supply an existing item that is not
     *       not currently on this list. */
    T * get_prev( T * x ) const {
        validate();
#if DLL2_STRICT_WALKING
        if ( !onthislist( x ))
            DLL2BAIL(( "ERROR get_prev ITEM is not on this list!\n" ));
#endif
        x->links[instance].validate();
        return x->links[instance].prev;
    }
};

#define DLL2_NUM_HASH_PRIMES 16
extern const int dll2_hash_primes[DLL2_NUM_HASH_PRIMES];

/** a data type for storing items in a hash, searchable by a key.
 * \note To use LListHash, type T must have a companion comparator class
 *       which provides hints about how to do hashing on the item. This
 *       comparator class must have the following structure:
 *       \code
 *       class DataTypeHashComparator {
 *       public:
 *          static int hash_key( T * item );
 *          static int hash_key( const Hash_Key_Type key );
 *          static bool hash_key_compare( T * item, const Hash_Key_Type key );
 *       }
 *       \endcode
 * \note In order to use LList, type T must have the following member: 
 *       \code LListLinks<T> links[ (number of lists) ]; \endcode
 * \ingroup dll2listtypes
 * \param T the type of the items to store in the hash
 * \param Hash_Key_Type the type of the key used to uniquely identify
 *        items in the hash
 * \param Hash_Key_Comparator the name of the companion comparator class
 *        which supplies the methods for indexing the hash by key type
 * \param instance the list instance number, basically the LListLinks index
 */
template <class T, class Hash_Key_Type,
          class Hash_Key_Comparator, int instance>
class LListHash {
private:
    short hashorder;
    short rehash_inprog;
    int hashsize;
    typedef LList <T,instance>  theLListHash;
    theLListHash * hash;
    static int hashv ( int key, int hs ) { return key % hs; }
    int count;
    theLListHash * rev_eng_hashind( T * t ) const {
        unsigned long min  = (unsigned long) & hash[ 0 ];
        unsigned long max  = (unsigned long) & hash[ hashsize-1 ];
        unsigned long mine = (unsigned long) t->links[instance].get_onlist();
        if ( mine == 0 || mine < min || mine > max ) return NULL;
        unsigned long off  = mine - min;
        if (( off % sizeof(theLListHash) ) != 0 )
            DLL2BAIL(( "LLISTHASH corrupt onlist pointer?\n" ));
        return (theLListHash *) mine;
    }
    void recalc_hash( void ) {
        if ( rehash_inprog )
            return;
        int average = count / hashsize;
        if ( average > 5  &&  hashorder < DLL2_NUM_HASH_PRIMES )
            rehash( dll2_hash_primes[ ++hashorder ] );
        else if ( average < 1  &&  hashorder > 0 )
            rehash( dll2_hash_primes[ --hashorder ] );
    }
    void rehash( int newhashsize ) {
        rehash_inprog = 1;   theLListHash * oldhash = hash;
        int oldhashsize = hashsize;  hashsize = newhashsize;
        hash = new theLListHash[ hashsize ];  count = 0;
        for ( int i = 0; i < oldhashsize; i++ )
            while ( T * t = oldhash[i].dequeue_head() )
                add( t );
        delete[] oldhash;
        rehash_inprog = 0;
    }
public:
    /** constructor, initializes head, tail, and checksum */
    LListHash ( void ) {
        hashorder = 0; rehash_inprog = 0;  count = 0;
        hashsize = dll2_hash_primes[ hashorder ];
        hash = new theLListHash[ hashsize ];
    }
    /** destructor, validates list is empty, error if not */
    ~LListHash ( void ) {
        for ( int i = 0; i < hashsize; i++ )
            if ( hash[i].get_cnt() != 0 )
                DLL2BAIL(( "LLISTHASH destructor: hash not empty!\n" ));
        delete[] hash;
    }
    /** utilize the comparator companion class methods to search the
     * hash for the supplied unique key, and return the matching item */
    T * find    ( const Hash_Key_Type key ) const {
        int h = hashv( Hash_Key_Comparator::hash_key( key ) & 0x7fffffff,
                       hashsize );
        T * ret;
        for ( ret = hash[h].get_head(); ret; ret = hash[h].get_next(ret))
            if ( Hash_Key_Comparator::hash_key_compare( ret, key ))
                break;
        return ret;
    }
    /** return the number of items currently on this list */
    int get_cnt( void ) const { return count; }
    /** add an item of type T to the hash, using the comparator companion
     * class to extract the key field from the item */
    void add( T * t ) {
        count++;
        int h = Hash_Key_Comparator::hash_key( t ) & 0x7fffffff;
        hash[ hashv( h, hashsize ) ].add( t );
        recalc_hash();
    }
    /** remove an item of type T from the hash, using the comparator companion
     * class to extract the key field from the item */
    void remove( T * t ) {
        count--;
        theLListHash * lst = rev_eng_hashind( t );
        if ( !lst )
            DLL2BAIL(( "LLISTHASH remove: not on this hash?\n" ));
        lst->remove( t );
        recalc_hash();
    }
    /** return true if the named item is currently on any list of
     * this instance number */
    bool onlist( const T * t ) const {
        return ( t->links[instance].onlist != 0 );
    }
    /** return true if the named item is currently on this hash */
    bool onthislist( const T * t ) const {
        theLListHash * lst = rev_eng_hashind( t );
        if ( !lst )
            return false;
        return true;
    }
};

/** a least-recently used list with promotion
 * \note the tail of the list represents the newest items, while the
 *       head of the item represents the oldest. the promote method
 *       moves the item from it's current position to the tail.
 * \note In order to use LList, type T must have the following member: 
 *       \code LListLinks<T> links[ (number of lists) ]; \endcode
 * \ingroup dll2listtypes
 * \param T the type of the items to store in the list
 * \param instance the list instance number, basically the LListLinks index
 */
template <class T, int instance>
class LListLRU {
private:
    LList <T,instance> list;
public:
    /** return the number of items currently on this list */
    int get_cnt( void ) const { return list.get_cnt(); }
    /** add an item of type T to tail of list */
    void add( T * t ) { list.add( t ); }
    /** remove an item from whereever it is in the list */
    void remove( T * t ) { list.remove( t ); }
    /** return the oldest member (at the head) */
    T * get_oldest( void ) const { return list.get_head(); }
    /** promote an item to the newest end of the list (at the tail) by
     * removing it from its current position and adding to the tail */
    void promote( T * t ) {
        if ( list.get_tail() != t ) {
            list.remove( t ); list.add( t );
        }
    }
    /** return true if the named item is currently on any list of
     * this instance number */
    bool onlist( const T * t ) const { return list.onlist( t ); }
    /** return true if the named item is currently on this list */
    bool onthislist( const T * t ) const { return list.onthislist( t ); }
    /** remove the head item (oldest) from the list and return it */
    T * dequeue_head( void ) { return list.dequeue_head(); }
    /** remove the tail item (newest) from the list and return it */
    T * dequeue_tail( void ) { return list.dequeue_tail(); }
    /** return the item that is at the head of the list (oldest) but don't
     * change the list */
    T * get_head( void ) const { return list.get_head(); }
    /** return the item that is at the tail of the list (newest) but don't
     * change the list */
    T * get_tail( void ) const { return list.get_tail(); }
    /** given an existing item which is currently on the list,
     * return the item which follows it (closer to tail and newer).
     * \note It is an error to supply an existing item that is not
     *       not currently on this list. */
    T * get_next( T * t ) const { return list.get_next( t ); }
    /** given an existing item which is currently on the list,
     * return the item which precedes it (closer to head and older).
     * \note It is an error to supply an existing item that is not
     *       not currently on this list. */
    T * get_prev( T * t ) const { return list.get_prev( t ); }
};

/** a compound container class with both a hash and a least recently used list.
 * \ingroup dll2listtypes
 * \note To use LListHash, type T must have a companion comparator class
 *       which provides hints about how to do hashing on the item. This
 *       comparator class must have the following structure:
 *       \code
 *       class DataTypeHashComparator {
 *       public:
 *          static int hash_key( T * item );
 *          static int hash_key( const Hash_Key_Type key );
 *          static bool hash_key_compare( T * item, const Hash_Key_Type key );
 *       }
 *       \endcode
 * \note In order to use LList, type T must have the following member: 
 *       \code LListLinks<T> links[ (number of lists) ]; \endcode
 * \param T the type of the items to store in the hash
 * \param Hash_Key_Type the type of the key used to uniquely identify
 *        items in the hash
 * \param Hash_Key_Comparator the name of the companion comparator class
 *        which supplies the methods for indexing the hash by key type
 * \param hash_instance the list instance number for the hash portion of this
 *        object, basically the LListLinks index
 * \param lru_instance the list instance number for the lru portion of this 
 *        object, basically the LListLinks index
 */
template <class T, 
          class Hash_Key_Type, class Hash_Key_Comparator,
          int hash_instance, int lru_instance>
class LListHashLRU {
private:
    LListHash <T, Hash_Key_Type, Hash_Key_Comparator, hash_instance> hash;
    LListLRU  <T, lru_instance > lru;
public:
    ~LListHashLRU( void ) { }
    /** search the hash portion of this object */
    T * find( const Hash_Key_Type key ) const { return hash.find( key ); }
    /** return the number of items stored in this object */
    int get_cnt( void ) const { return lru.get_cnt(); }
    /** add an item to both the hash and the lru */
    void add( T * t ) { hash.add( t ); lru.add( t ); }
    /** remove an item from both the hash and the lru */
    void remove( T * t ) { hash.remove( t ); lru.remove( t ); }
    /** promote the item in the lru */
    void promote( T * t ) { lru.promote( t ); }
    /** return true if the item is on any list with the same instance
     * number as the lru_instance */
    bool onlist( const T * t ) const { return lru.onlist( t ); }
    /** return true if the item is stored in this object */
    bool onthislist( const T * t ) const { return lru.onthislist( t ); }
    /** remove the item at the head of the lru (oldest) and return it */
    T * dequeue_lru_head( void ) {
        T * ret = lru.get_head();
        if ( ret ) remove( ret );
        return ret;
    }
    /** remove the item at the tail of the lru (newest) and return it */
    T * dequeue_lru_tail( void ) {
        T * ret = lru.get_tail();
        if ( ret ) remove( ret );
        return ret;
    }
    /** return the item at the head of the lru (oldest) but don't modify
     * either the hash or the lru */
    T * get_lru_head ( void ) const { return lru.get_head(); }
    /** return the item at the tail of the lru (newest) but don't modify
     * either the hash or the lru */
    T * get_lru_tail ( void ) const { return lru.get_tail(); }
    /** given an existing item which is on the lru, return the next item
     * (newer) */
    T * get_lru_next( T * t ) const { return lru.get_next(t); }
    /** given an existing item which is on the lru, return the previous item
     * (older) */
    T * get_lru_prev( T * t ) const { return lru.get_prev(t); }
// hash stats
    /** modify the size of the hash array to another size, note this
     * is an internal method */
    void rehash( int newhashsize ) { hash.rehash( newhashsize ); }
};

/** \todo document LListOrderedQ */
/** \todo document btree API */

// to use LListOrderedQ, type T must have the following member:
//    - int ordered_queue_key

template <class T, int instance>
class LListOrderedQ {
private:
    LList <T,instance> oq;
public:
    int get_cnt( void ) const { return oq.get_cnt(); }
    void add( T * t, int new_oq_key ) {
        T * cur, * prev;
        for ( prev = 0, cur = oq.get_head();
              cur;
              prev = cur, cur = oq.get_next( cur ))
        {
            if ( new_oq_key < cur->ordered_queue_key )
            {
                cur->ordered_queue_key -= new_oq_key;
                break;
            }
            new_oq_key -= cur->ordered_queue_key;
        }
        t->ordered_queue_key = new_oq_key;
        if ( cur )
            oq.add_before( t, cur ); // in the middle of the list
        else if ( prev )
            oq.add_after( t, prev ); // after the last item of the list
        else
            oq.add( t );             // the list was empty
    }
    void remove( T * t ) {
        T * n = oq.get_next( t );
        oq.remove( t );
        if ( n )
            n->ordered_queue_key += t->ordered_queue_key;
    }
    T * get_head( void ) const { return oq.get_head(); }
    T * get_next( T * t ) const { return oq.get_next( t ); }
    bool onlist( const T * t ) const { return oq.onlist( t ); }
    bool onthislist( const T * t ) const { return oq.onthislist( t ); }
};

#if DLL2_INCLUDE_BTREE
#include "dll2_btree.H"
#endif

#endif /* __DLL2_H_ */
