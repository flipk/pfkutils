
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
types.  The following diagram shows the relationship between the objects.

\dot 

digraph FileMGTStructure {
  graph [rankdir=LR];
  node [shape=record, fontname=Helvetica, fontsize=10];
  edge [arrowhead="open", style="solid"];

  PageIO     [label="Page\nIO"           URL="\ref PageIO"          ];
  PageCache  [label="Page\nCache"        URL="\ref PageCache"       ];
  BlockCache [label="Block\nCache"       URL="\ref BlockCache"      ];
  FileBlock  [label="FileBlock"         URL="\ref FileBlock"       ];
  BTree      [label="B-Tree"            URL="\ref Btree"           ];

  Pages      [label="Pages"   shape=oval ];
  Blocks     [label="Blocks"  shape=oval ];
  FBlocks    [label="Blocks"  shape=oval ];
  BTRecs     [label="Records" shape=oval ];

  BTree      ->  BTRecs      ;
  FileBlock  ->  FBlocks     ;
  BlockCache ->  Blocks      ;
  PageCache  ->  Pages       ;

  BTree      ->  FileBlock   ;
  FileBlock  ->  BlockCache  ;
  BlockCache ->  PageCache   ;
  PageCache  ->  PageIO      ;

  edge [style="dashed"];

  BTRecs  -> FBlocks ;
  FBlocks -> Blocks  ;
  Blocks  -> Pages   ;

  { rank=same; BTree      BTRecs  }
  { rank=same; FileBlock  FBlocks }
  { rank=same; BlockCache Blocks  }
  { rank=same; PageCache  Pages   }
}

\enddot

  Click on the following links to read about each of them.

<ul>
<li> \ref PageIO (see classes PageIO and PageIOFileDescriptor)
<li> \ref PageCache (see classes PageCachePage and PageCache)
<li> \ref BlockCache (see classes BlockCacheBlock, BCB, and BlockCache)
<li> \ref FileBlock (see classes FileBlock, FileBlockInterface, FileBlockLocal)
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
