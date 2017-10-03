
/** \file BtreeNode.C
 * \brief implementation of BTNode object
 * \author Phillip F Knaack
 */

#include "Btree.H"
#include "Btree_internal.H"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

BTNode :: BTNode( FileBlockInterface * _fbi, int _btorder, UINT32 * _fbnp )
{
    fbi = _fbi;
    btorder = _btorder;
    fbnp = _fbnp;

    UINT32 fbn = *fbnp;

    BTNodeDisk  btn(fbi);
    btn.get( fbn );

    
}
