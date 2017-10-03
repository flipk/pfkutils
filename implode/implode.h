/***************************************************************************

Header File Name:     implode2.h
 
Description:           
  This module contains the structures and equates used by AsiWare's
  implode2 algorithm.

****************************************************************************

Copyright Motorola, Inc., 1993  All Rights Reserved

This material contains information which is proprietary and confidential to
Motorola, Inc.  It is disclosed to the customer solely for the following
purpose; namely, for the purpose of enabling the customer to evaluate the
operation of the product delivered to the customer.  Customer is not to
reproduce, copy, divulge or sell all or any part thereof without prior
written consent of an authorized representative of Motorola.

*****************************************************************************/

/*
PKWARE OS/2 Data Compression Library
Copyright 1990 PKWARE Inc. and ASi  All Rights Reserved.
*/

typedef int (*lzw_io_func)( void * arg, unsigned char *, int );

int lzw_Implode( lzw_io_func ReadBuf, lzw_io_func WriteBuf, void * arg,
                 int Mode, int DictSize, int CmpLevel );
int lzw_Explode( lzw_io_func ReadBuf, lzw_io_func WriteBuf, void * arg );

/* mode values */
#define CMP_BINARY             0
#define CMP_ASCII              1

/* return values */
#define CMP_NO_ERROR           0
#define CMP_INVALID_DICTSIZE   1
#define CMP_INVALID_MODE       2
#define CMP_INVALID_CMPLEVEL   3

#define CMP_BAD_DATA           4
#define CMP_ABORT              5


/*

call tree:

lzw_Implode
    FillWindow
        GetBuf
    FindRep
    OutputBits
        PutBuf
    PutBuf

lzw_Explode
    GetBuf
    PutBuf
    GenAscTabs
    GenDecodeTabs
    DecodeLit
        WasteBits
            GetBuf
    DecodeDist
        WasteBits
            GetBuf

*/
