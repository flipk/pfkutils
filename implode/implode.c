
/***************************************************************************

Function Name: Implode
Abstract: AsiWare Compression algorithm
Created:  3/31/93 (added prologue)
Author:   AsiWare

Input Data Parameters:
  int (*ReadBuff)()     Pointer to a user defined read function.
  int (*WriteBuff)()    Pointer to a user defined write function.
  int mode              Pointer to mode specifier. (Binary or ASCII)
  int DictSize          Pointer to dictionary size
                         (1024, 2048, or 4096)
  int Cmplevel          Pointer to compress level
                         (0=fastest/larger ... 9 = slowest/smaller)

Output Data Parameters: none

Return Values:          refer to implode.h

Calls:                  none                

Description:            This is the collection of modules that make up the
                        implode compression algorithm by AsiWare Inc.  The
                        implode algorithm is a variant on the Lempil Ziv
                        algorithm.  It is based on string pattern matching.

***************************************************************************/

#include <string.h>

#define SUCF static const unsigned char
#define SUF static const unsigned short


SUCF DistBits[] = {
    2, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

SUCF DistCode[] = {
    0x003,0x00d,0x005,0x019,0x009,0x011,0x001,0x03e,
    0x01e,0x02e,0x00e,0x036,0x016,0x026,0x006,0x03a,
    0x01a,0x02a,0x00a,0x032,0x012,0x022,0x042,0x002,
    0x07c,0x03c,0x05c,0x01c,0x06c,0x02c,0x04c,0x00c,
    0x074,0x034,0x054,0x014,0x064,0x024,0x044,0x004,
    0x078,0x038,0x058,0x018,0x068,0x028,0x048,0x008,
    0x0f0,0x070,0x0b0,0x030,0x0d0,0x050,0x090,0x010,
    0x0e0,0x060,0x0a0,0x020,0x0c0,0x040,0x080,0x000
};

SUCF ExLenBits[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8
};

SUF LenBase[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8,10,14,22,38,70,134,262
};

SUCF LenBits[] = {
    3, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7
};

SUCF LenCode[]= {
    0x05, 0x03, 0x01, 0x06, 0x0A, 0x02, 0x0C, 0x14,
    0x04, 0x18, 0x08, 0x30, 0x10, 0x20, 0x40, 0x00
};

SUCF ChBitsAsc[] = {
    11, 12, 12, 12, 12, 12, 12, 12, 12,  8,  7, 12, 12,  7, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 12, 12, 12, 12, 12,
    04, 10,  8, 12, 10, 12, 10,  8,  7,  7,  8,  9,  7,  6,  7,  8,
    07,  6,  7,  7,  7,  7,  8,  7,  7,  8,  8, 12, 11,  7,  9, 11,
    12,  6,  7,  6,  6,  5,  7,  8,  8,  6, 11,  9,  6,  7,  6,  6,
    07, 11,  6,  6,  6,  7,  9,  8,  9,  9, 11,  8, 11,  9, 12,  8,
    12,  5,  6,  6,  6,  5,  6,  6,  6,  5, 11,  7,  5,  6,  5,  5,
    06, 10,  5,  5,  5,  5,  8,  7,  8,  8, 10, 11, 11, 12, 12, 12,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    13, 12, 13, 13, 13, 12, 13, 13, 13, 12, 13, 13, 13, 13, 12, 13,
    13, 13, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13
};

SUF ChCodeAsc[]= {
    0x490 , 0xfe0 , 0x7e0 , 0xbe0 , 0x3e0 , 0xde0 , 0x5e0 , 0x9e0 ,
    0x1e0 , 0xb8  , 0x62  , 0xee0 , 0x6e0 , 0x22  , 0xae0 , 0x2e0 ,
    0xce0 , 0x4e0 , 0x8e0 , 0xe0  , 0xf60 , 0x760 , 0xb60 , 0x360 ,
    0xd60 , 0x560 , 0x1240, 0x960 , 0x160 , 0xe60 , 0x660 , 0xa60 ,
    0xf   , 0x250 , 0x38  , 0x260 , 0x50  , 0xc60 , 0x390 , 0xd8  ,
    0x42  , 0x2   , 0x58  , 0x1b0 , 0x7c  , 0x29  , 0x3c  , 0x98  ,
    0x5c  , 0x9   , 0x1c  , 0x6c  , 0x2c  , 0x4c  , 0x18  , 0xc   ,
    0x74  , 0xe8  , 0x68  , 0x460 , 0x90  , 0x34  , 0xb0  , 0x710 ,
    0x860 , 0x31  , 0x54  , 0x11  , 0x21  , 0x17  , 0x14  , 0xa8  ,
    0x28  , 0x1   , 0x310 , 0x130 , 0x3e  , 0x64  , 0x1e  , 0x2e  ,
    0x24  , 0x510 , 0xe   , 0x36  , 0x16  , 0x44  , 0x30  , 0xc8  ,
    0x1d0 , 0xd0  , 0x110 , 0x48  , 0x610 , 0x150 , 0x60  , 0x88  ,
    0xfa0 , 0x7   , 0x26  , 0x6   , 0x3a  , 0x1b  , 0x1a  , 0x2a  ,
    0xa   , 0xb   , 0x210 , 0x4   , 0x13  , 0x32  , 0x3   , 0x1d  ,
    0x12  , 0x190 , 0xd   , 0x15  , 0x5   , 0x19  , 0x8   , 0x78  ,
    0xf0  , 0x70  , 0x290 , 0x410 , 0x10  , 0x7a0 , 0xba0 , 0x3a0 ,
    0x240 , 0x1c40, 0xc40 , 0x1440, 0x440 , 0x1840, 0x840 , 0x1040,
    0x40  , 0x1f80, 0xf80 , 0x1780, 0x780 , 0x1b80, 0xb80 , 0x1380,
    0x380 , 0x1d80, 0xd80 , 0x1580, 0x580 , 0x1980, 0x980 , 0x1180,
    0x180 , 0x1e80, 0xe80 , 0x1680, 0x680 , 0x1a80, 0xa80 , 0x1280,
    0x280 , 0x1c80, 0xc80 , 0x1480, 0x480 , 0x1880, 0x880 , 0x1080,
    0x80  , 0x1f00, 0xf00 , 0x1700, 0x700 , 0x1b00, 0xb00 , 0x1300,
    0xda0 , 0x5a0 , 0x9a0 , 0x1a0 , 0xea0 , 0x6a0 , 0xaa0 , 0x2a0 ,
    0xca0 , 0x4a0 , 0x8a0 , 0xa0  , 0xf20 , 0x720 , 0xb20 , 0x320 ,
    0xd20 , 0x520 , 0x920 , 0x120 , 0xe20 , 0x620 , 0xa20 , 0x220 ,
    0xc20 , 0x420 , 0x820 , 0x20  , 0xfc0 , 0x7c0 , 0xbc0 , 0x3c0 ,
    0xdc0 , 0x5c0 , 0x9c0 , 0x1c0 , 0xec0 , 0x6c0 , 0xac0 , 0x2c0 ,
    0xcc0 , 0x4c0 , 0x8c0 , 0xc0  , 0xf40 , 0x740 , 0xb40 , 0x340 ,
    0x300 , 0xd40 , 0x1d00, 0xd00 , 0x1500, 0x540 , 0x500 , 0x1900,
    0x900 , 0x940 , 0x1100, 0x100 , 0x1e00, 0xe00 , 0x140 , 0x1600,
    0x600 , 0x1a00, 0xe40 , 0x640 , 0xa40 , 0xa00 , 0x1200, 0x200 ,
    0x1c00, 0xc00 , 0x1400, 0x400 , 0x1800, 0x800 , 0x1000, 0x0
};

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef NULL
#define NULL ((void *)0)                            
#endif

#define UCHARP_TO_USHORT(x) (((USHORT) *(x) << 8) | (USHORT) *((x) + 1))

#define SHORT         short            /* 16 bits, signed   */
#define USHORT        unsigned short   /* 16 bits, unsigned */
#define CHAR          char             /* 8 bits,  signed   */
#define UCHAR         unsigned char    /* 8 bits,  unsigned */
#define INT           int
#define UINT          unsigned int
#define VOID          void
#define MAX_DICT_SIZE 4096
#define EXP_BUFSIZE   4096
#define OUT_SIZE      4096
#define HASH_SIZE     2304
#define MINREP        2
#define SIZE_DIST     64
#define SIZE_LEN      16
#define MAXREP       (MINREP+(8*1)+2+4+8+16+32+64+128+256-4)
#define SIZE_LIT     (256+MAXREP+2)
#define EOF_CODE     (SIZE_LIT-1)
#define ABORT_CODE   (EOF_CODE+1)

#include "implode.h"

static const struct
{
    UINT LookAhead;      /* Max length allowed before looking at next match */
    UINT FindRepLoops;   /* Max # of match attempts in FindRep routine */
}
CmpConfig[10] = {
    {  3,    2  },            /* Level 0 - Maximum Speed */
    {  3,    4  },
    {  4,    8  },
    {  5,   16  },
    {  6,   32  },
    {  8,   64  },            /* Level 5 - Normal Compression */
    { 10,   96  },
    { 12,  128  },
    { 15,  512  },
    { 20, 1024  }             /* Level 9 - Maximum Compression */
};

struct LZW_CMP_DATA
{
    UINT     OutPtr;
    UINT     OutPos;
    UCHAR   *InPtr;
    UINT     ExtDistBits;
    UINT     ExtDistMask;
    UINT     NextMask;
    UINT     FindRepLoops;
    UINT     LookAhead;
    USHORT   DictSize;
    UCHAR    Mode;
    UCHAR    InputDone;
    UCHAR    DistBits [SIZE_DIST];
    UCHAR    DistCode [SIZE_DIST];
    UCHAR    LitBits  [SIZE_LIT];
    USHORT   LitCode  [SIZE_LIT];
    USHORT   Hash     [HASH_SIZE];
    UCHAR    OutBuf   [OUT_SIZE+2];
    UCHAR    Buffer   [MAX_DICT_SIZE*2 + MAXREP];
    USHORT   Next     [MAX_DICT_SIZE];

    lzw_io_func GetBuf;
    lzw_io_func PutBuf;
    void * arg;
};

struct LZW_EXP_DATA
{
    UINT    Distance;
    UINT    Mode;
    UINT    OutPtr;
    UINT    ExtDistBits;
    UINT    ExtDistMask;
    UINT    LookAhead;
    UINT    LookBits;
    UINT    InPtr;
    UINT    InBufCnt;
    UCHAR   OutBuf[(EXP_BUFSIZE*2)+MAXREP];
    UCHAR   InBuf[OUT_SIZE];
    UCHAR   DistDecode[256];
    UCHAR   LenDecode[256];
    UCHAR   ChLow[256];
    UCHAR   ChMid1[256];
    UCHAR   ChMid2[128];
    UCHAR   ChHi[256];
    UCHAR   ChBits[256];
    UCHAR   DistBits[SIZE_DIST];
    UCHAR   LenBits[SIZE_LEN];
    UCHAR   ExLenBits[SIZE_LEN];
    USHORT  LenBase[SIZE_LEN];

    lzw_io_func GetBuf;
    lzw_io_func PutBuf;
    void * arg;
};

static UINT FindRep( struct LZW_CMP_DATA *Cmp,
                     UCHAR * Cur, UINT Lim, UINT * Dist );
static UINT FillWindow( struct LZW_CMP_DATA *Cmp,
                        UCHAR ** CurPos, UCHAR ** Lim );
static VOID OutputBits( struct LZW_CMP_DATA *Cmp, UINT Cnt, UINT Code );
static UINT DecodeLit( struct LZW_EXP_DATA *Exp );
static UINT DecodeDist( struct LZW_EXP_DATA *Exp, UINT Len );
static UINT WasteBits( struct LZW_EXP_DATA *Exp, UINT Bits );
static VOID GenDecodeTabs( UINT Len, UCHAR *Bits,
                           const UCHAR *Code, UCHAR *Decode );
static VOID GenAscTabs( struct LZW_EXP_DATA *Exp );

int
lzw_Implode( lzw_io_func ReadBuf, lzw_io_func WriteBuf, void * arg,
             int Mode, int DictSize, int CmpLevel )
{
    UINT   i, j, k, fillret;
    struct LZW_CMP_DATA   Cmp;
    UINT   MatchLen, MatchDist, PrevLen, PrevDist, PrevAvail, HashVal, Pos;
    UCHAR  *Cur, *Lim;

    Cmp.GetBuf = ReadBuf;
    Cmp.PutBuf = WriteBuf;
    Cmp.arg = arg;
    Cmp.DictSize = (USHORT)DictSize;
    Cmp.NextMask = DictSize - 1;
    Cmp.Mode = Mode;

    if (CmpLevel > 9)
        return CMP_INVALID_CMPLEVEL;

    Cmp.LookAhead = CmpConfig[CmpLevel].LookAhead;
    Cmp.FindRepLoops = CmpConfig[CmpLevel].FindRepLoops;
    Cmp.InputDone = FALSE;
    Cmp.ExtDistBits = 4;
    Cmp.ExtDistMask = 0xF;

    switch (DictSize)
    {
    case 4096:
        Cmp.ExtDistBits++;
        Cmp.ExtDistMask |= 0x20;
        /* fall thru */
    case 2048:
        Cmp.ExtDistBits++;
        Cmp.ExtDistMask |= 0x10;
        /* fall thru */
    case 1024:
        break;
    default:
        return CMP_INVALID_DICTSIZE;
    }

    switch (Mode)
    {
    case CMP_ASCII:
        for (k=0; k<256; k++)
        {
            Cmp.LitBits[k] = (UCHAR)(ChBitsAsc[k] + 1);
            Cmp.LitCode[k] = (USHORT)(ChCodeAsc[k] << 1);
        }
        break;
    case CMP_BINARY:
        for (k=0; k<256; k++)
        {
            Cmp.LitBits[k] = 9;
            Cmp.LitCode[k] = (USHORT)(k << 1);
        }
        break;
    default:
        return CMP_INVALID_MODE;
    }

    for (i=0; i<16; i++)
    {
        for (j=0; j < ( 1<<ExLenBits[i]); j++, k++)
        {
            Cmp.LitBits[k] = (UCHAR)(LenBits[i] + ExLenBits[i] + 1);
            Cmp.LitCode[k] =
                (USHORT)((j << (LenBits[i] + 1)) |
                         (LenCode[i] << 1) | 1);
        }
    }

    memcpy((char *)Cmp.DistCode,(char *)DistCode,sizeof(DistCode));
    memcpy((char *)Cmp.DistBits,(char *)DistBits,sizeof(DistBits));

#define HASH(x) (4 * (x)[0] + 5 * (x)[1])

#define UPDATE_HASH(c,p) \
    do { \
        HashVal = HASH(c); \
        Cmp.Next[ (p) & Cmp.NextMask ] = Cmp.Hash[HashVal]; \
        Cmp.Hash[HashVal] = (USHORT)(p); \
    } while ( 0 )

    Cmp.OutBuf[0] = (UCHAR)Cmp.Mode;       /* Output Mode & Dist bytes */
    Cmp.OutBuf[1] = (UCHAR)Cmp.ExtDistBits;
    Cmp.OutPtr = 2;
    memset((char *)Cmp.OutBuf+2, 0, OUT_SIZE);
    Cmp.OutPos = 0;

    memset((char *)Cmp.Hash, 0xFF, HASH_SIZE * sizeof(USHORT));
    MatchLen = MINREP - 1;
    MatchDist = 0;
    PrevAvail = FALSE;
    Cur = Cmp.InPtr = Cmp.Buffer;

    do {
        fillret = FillWindow( &Cmp, &Cur, &Lim );
        Pos = (USHORT)(Cur - Cmp.Buffer);

        while (Cur < Lim)
        {
            UPDATE_HASH(Cur, Pos);

            PrevLen = MatchLen;
            PrevDist = MatchDist;

            if (PrevLen < Cmp.LookAhead)
                MatchLen = FindRep(&Cmp, Cur, (UINT)(Lim - Cur), &MatchDist);

            if ((MatchLen <= PrevLen) &&
                (PrevLen > MINREP ||
                 (PrevLen == MINREP && PrevDist < 256)))
            {
                OutputBits( &Cmp,
                            Cmp.LitBits[PrevLen-MINREP+256],
                            Cmp.LitCode[PrevLen-MINREP+256] );

                if (PrevLen == MINREP)
                {
                    OutputBits( &Cmp,
                                Cmp.DistBits[PrevDist >> 2],
                                Cmp.DistCode[PrevDist >> 2] );
                    OutputBits( &Cmp, 2, PrevDist & 3 );
                }
                else
                {
                    OutputBits( &Cmp,
                                Cmp.DistBits[PrevDist >> Cmp.ExtDistBits],
                                Cmp.DistCode[PrevDist >> Cmp.ExtDistBits] );
                    OutputBits( &Cmp,
                                Cmp.ExtDistBits,
                                PrevDist & Cmp.ExtDistMask );
                }

                --PrevLen;
                while (--PrevLen)
                {
                    ++Cur;
                    ++Pos;
                    UPDATE_HASH(Cur, Pos);
                }

                PrevAvail = FALSE;
                MatchLen = 0;
            }
            else if (PrevAvail)
                OutputBits( &Cmp,
                            Cmp.LitBits[*(Cur-1)],
                            Cmp.LitCode[*(Cur-1)]);
            else
                PrevAvail = TRUE;

            ++Cur;
            ++Pos;
        }
    } while ( fillret != 0 );

    if (PrevAvail)
        OutputBits( &Cmp, Cmp.LitBits[*(Cur-1)], Cmp.LitCode[*(Cur-1)] );

    OutputBits( &Cmp, Cmp.LitBits[EOF_CODE], Cmp.LitCode[EOF_CODE] );

    if (Cmp.OutPos != 0)
        ++Cmp.OutPtr;
    Cmp.PutBuf( Cmp.arg, Cmp.OutBuf, Cmp.OutPtr );

    return CMP_NO_ERROR;
}

static UINT
FillWindow( struct LZW_CMP_DATA *Cmp, UCHAR ** CurPos, UCHAR ** Lim )
{
    UINT i, Request, InCnt;
    SHORT j, Adjust;
    UCHAR *FullPtr = &Cmp->Buffer[(MAX_DICT_SIZE*2) - Cmp->DictSize];

    if (Cmp->InputDone == TRUE)
        return 0;

    if (*CurPos >= FullPtr)
    {
        Adjust = (SHORT)(FullPtr - Cmp->Buffer);

        memcpy( (char *)Cmp->Buffer,
                (char *)FullPtr,
                Cmp->DictSize + MAXREP );

        *CurPos -= Adjust;
        Cmp->InPtr -= Adjust;

        for (i=0; i<HASH_SIZE; i++)
        {
            j = Cmp->Hash[i];
            Cmp->Hash[i] = (SHORT)((j >= Adjust) ? j - Adjust : -1);
        }

        for (i=0; i<Cmp->DictSize; i++)
        {
            j = Cmp->Next[i];
            Cmp->Next[i] = (SHORT)((j >= Adjust) ? j - Adjust : -1);
        }
    }

    Request = (UINT)(&Cmp->Buffer[(MAX_DICT_SIZE * 2) + MAXREP] - Cmp->InPtr);
    InCnt = 0;

    do {

        i = Cmp->GetBuf( Cmp->arg, Cmp->InPtr, Request );
        if ( i == 0 )
        {
            *Lim = Cmp->InPtr;
            Cmp->InputDone = TRUE;
            return InCnt;
        }

        InCnt += i;
        Cmp->InPtr += i;
        Request -= i;

    } while (Request);

    *Lim = Cmp->InPtr - MAXREP;
    Cmp->InputDone = FALSE;

    return InCnt;
}

static UINT
FindRep( struct LZW_CMP_DATA *Cmp, UCHAR * Cur, UINT iLim, UINT * iDist)
{
    UCHAR  *Pat, *Ptr;
    UINT   MaxItems = Cmp->FindRepLoops, MatchLen = 1, Len;
    SHORT  mPos, iLimPos;
    USHORT LastWord;

    if (iLim > MAXREP)
        iLim = MAXREP;
    else if (iLim < MINREP)
        return 1;

    mPos = (SHORT)(Cur - Cmp->Buffer);
    iLimPos = (SHORT)((mPos > Cmp->DictSize) ? mPos - Cmp->DictSize : -1);
    LastWord = UCHARP_TO_USHORT(Cur);
    MatchLen = 1;

    do {

        if ((mPos = (SHORT) Cmp->Next[mPos & Cmp->NextMask]) <= iLimPos)
            break;

        Ptr = &Cmp->Buffer[mPos];

        if (UCHARP_TO_USHORT(Ptr+MatchLen-1) == LastWord &&
            UCHARP_TO_USHORT(Ptr) == UCHARP_TO_USHORT(Cur))
        {
            Pat = Cur;
            Len = 2; Ptr+=2; Pat+=2;
            while (*Ptr++ == *Pat++ && ++Len < iLim);
            if (Len > MatchLen)
            {
                *iDist = (UINT)(Cur - &Cmp->Buffer[mPos] - 1);
                if ((MatchLen = Len) >= iLim)
                    return iLim;
                LastWord = UCHARP_TO_USHORT(Cur+MatchLen-1);
            }
        }

    } while (--MaxItems);

    return MatchLen;
}

static VOID
OutputBits(struct LZW_CMP_DATA *Cmp, UINT Cnt, UINT Code)
{
    unsigned p;

    if (Cnt > 8)
    {
        OutputBits(Cmp,8,Code);
        Cnt -= 8;
        Code >>= 8;
    }

    *(Cmp->OutBuf+Cmp->OutPtr) |= Code << (p = Cmp->OutPos);

    if ((Cmp->OutPos = p + Cnt) > 8)
    {
        ++Cmp->OutPtr;
        *(Cmp->OutBuf+Cmp->OutPtr) = (UCHAR)(Code >> (8 - p));
        Cmp->OutPos &= 7;
    }
    else if ((Cmp->OutPos &= 7) == 0)
        ++Cmp->OutPtr;

    if (Cmp->OutPtr >= OUT_SIZE)
    {
        UCHAR s1, s2;
        UINT i = OUT_SIZE;

        Cmp->PutBuf( Cmp->arg, Cmp->OutBuf, i );

        s1 = Cmp->OutBuf[Cmp->OutPtr];
        s2 = Cmp->OutBuf[OUT_SIZE];
        Cmp->OutPtr -= OUT_SIZE;
        memset((char *)Cmp->OutBuf,0,OUT_SIZE+2);

        if (Cmp->OutPtr > 0)
            Cmp->OutBuf[0] = s2;
        if (Cmp->OutPos != 0)
            Cmp->OutBuf[Cmp->OutPtr] = s1;
    }
}

int
lzw_Explode( lzw_io_func ReadBuf, lzw_io_func WriteBuf, void * arg )
{
    struct LZW_EXP_DATA _Exp;
    struct LZW_EXP_DATA *Exp;
    UINT Len, Dist;
    UCHAR *Src, *Dest;

    Exp = &_Exp;
    Exp->GetBuf = ReadBuf;
    Exp->PutBuf = WriteBuf;
    Exp->arg = arg;

    Exp->InPtr = OUT_SIZE;
    Exp->InBufCnt = Exp->GetBuf( Exp->arg, Exp->InBuf, Exp->InPtr );
    if ( Exp->InBufCnt <= 4 )
        return CMP_BAD_DATA;

    Exp->Mode = Exp->InBuf[0];
    Exp->ExtDistBits = Exp->InBuf[1];
    Exp->LookAhead = Exp->InBuf[2];
    Exp->LookBits = 0;
    Exp->InPtr = 3;

    if (Exp->ExtDistBits < 4 || Exp->ExtDistBits > 6)
        return CMP_INVALID_DICTSIZE;
    Exp->ExtDistMask = 0xFFFF >> (16 - Exp->ExtDistBits);

    switch (Exp->Mode)
    {
    case CMP_ASCII:
        memcpy((char *)Exp->ChBits,(char *)ChBitsAsc,sizeof(ChBitsAsc));
        GenAscTabs(Exp);
        break;
    case CMP_BINARY:
        break;
    default:
        return CMP_INVALID_MODE;
    }

    memcpy( (char *)Exp->LenBits,   (char *)LenBits,   sizeof(LenBits)   );
    GenDecodeTabs( SIZE_LEN,  Exp->LenBits,  LenCode,  Exp->LenDecode    );
    memcpy( (char *)Exp->ExLenBits, (char *)ExLenBits, sizeof(ExLenBits) );
    memcpy( (char *)Exp->LenBase,   (char *)LenBase,   sizeof(LenBase)   );
    memcpy( (char *)Exp->DistBits,  (char *)DistBits,  sizeof(DistBits)  );
    GenDecodeTabs( SIZE_DIST, Exp->DistBits, DistCode, Exp->DistDecode   );

    Exp->OutPtr = EXP_BUFSIZE;
    while ((Len = DecodeLit(Exp)) < EOF_CODE)
    {
        if (Len < 256)
            Exp->OutBuf[Exp->OutPtr++] = (UCHAR)Len;
        else
        {
            Len -= (256 - MINREP);
            if ((Dist = DecodeDist(Exp,Len)) == 0)
                return CMP_ABORT;

            Dest = &Exp->OutBuf[Exp->OutPtr];
            Src = Dest - Dist;
            Exp->OutPtr += Len;
            do {
                *Dest++ = *Src++;
            } while (--Len);
        }

        if (Exp->OutPtr >= EXP_BUFSIZE*2)
        {
            Len = EXP_BUFSIZE;
            Exp->PutBuf( Exp->arg, &Exp->OutBuf[EXP_BUFSIZE], Len );
            memcpy( (char *)Exp->OutBuf,
                    (char *)&Exp->OutBuf[EXP_BUFSIZE],
                    Exp->OutPtr-EXP_BUFSIZE );
            Exp->OutPtr -= EXP_BUFSIZE;
        }
    }

    Len = Exp->OutPtr - EXP_BUFSIZE;
    Exp->PutBuf( Exp->arg, &Exp->OutBuf[EXP_BUFSIZE], Len );

    return CMP_NO_ERROR;
}

static UINT
DecodeLit( struct LZW_EXP_DATA *Exp )
{
    UINT LitChar, i;

    if (Exp->LookAhead & 1)          /* Length found */
    {
        if (WasteBits(Exp,1))
            return ABORT_CODE;
        LitChar = Exp->LenDecode[Exp->LookAhead & 0xFF];
        if (WasteBits(Exp,Exp->LenBits[LitChar]))
            return ABORT_CODE;
        if (Exp->ExLenBits[LitChar])
        {
            i = Exp->LookAhead & ((1 << Exp->ExLenBits[LitChar]) - 1);
            if (WasteBits(Exp,Exp->ExLenBits[LitChar]))
                return ABORT_CODE;
            LitChar = Exp->LenBase[LitChar] + i;
        }
        LitChar += 256;
    }
    else                        /* Character found */
    {
        if (WasteBits(Exp,1))
            return ABORT_CODE;
        if (Exp->Mode == CMP_BINARY)
        {
            LitChar = Exp->LookAhead & 0xFF;
            if (WasteBits(Exp,8))
                return ABORT_CODE;
        }
        else
        {
            if (Exp->LookAhead & 0xFF)           /* Low/Mid tab */
            {
                if ((LitChar = Exp->ChLow[Exp->LookAhead & 0xFF]) == 0xFF)
                {
                    if (Exp->LookAhead & 0x3F)     /* Mid tab 1 */
                    {
                        if (WasteBits(Exp,4))
                            return ABORT_CODE;
                        LitChar = Exp->ChMid1[Exp->LookAhead & 0xFF];
                    }
                    else                           /* Mid tab 2 */
                    {
                        if (WasteBits(Exp,6))
                            return ABORT_CODE;
                        LitChar = Exp->ChMid2[Exp->LookAhead & 0x7F];
                    }
                }
            }
            else                                 /* High tab */
            {
                if (WasteBits(Exp,8))
                    return ABORT_CODE;
                LitChar = Exp->ChHi[Exp->LookAhead & 0xFF];
            }
            if (WasteBits(Exp,Exp->ChBits[LitChar]))
                return ABORT_CODE;
        }
    }
    return LitChar;
}

static UINT
DecodeDist( struct LZW_EXP_DATA *Exp, UINT Len )
{
    UINT Dist;

    Dist = Exp->DistDecode[Exp->LookAhead & 0xFF];
    if (WasteBits(Exp,Exp->DistBits[Dist]))
        return 0;

    if (Len == MINREP)
    {
        Dist <<= 2;
        Dist |= Exp->LookAhead & 3;
        if (WasteBits(Exp,2))
            return 0;
    }
    else
    {
        Dist <<= Exp->ExtDistBits;
        Dist |= Exp->LookAhead & Exp->ExtDistMask;
        if (WasteBits(Exp,Exp->ExtDistBits))
            return 0;
    }

    return Dist+1;
}

static UINT
WasteBits(struct LZW_EXP_DATA *Exp, UINT Bits)
{
    if (Bits <= Exp->LookBits)
    {
        Exp->LookAhead >>= Bits;
        Exp->LookBits -= Bits;
        return 0;
    }

    Exp->LookAhead >>= Exp->LookBits;

    if (Exp->InPtr == Exp->InBufCnt)
    {
        Exp->InPtr = OUT_SIZE;
        Exp->InBufCnt = Exp->GetBuf( Exp->arg, Exp->InBuf, Exp->InPtr );
        if ( Exp->InBufCnt == 0 )
            return 1;
        Exp->InPtr = 0;
    }

    Exp->LookAhead |= (Exp->InBuf[Exp->InPtr])<<8;
    ++Exp->InPtr;
    Exp->LookAhead >>= (Bits - Exp->LookBits);
    Exp->LookBits = 8-(Bits-Exp->LookBits);
    return 0;
}

static VOID
GenDecodeTabs(UINT Len, UCHAR *Bits, const UCHAR *Code, UCHAR *Decode)
{
    UINT j, Incr;
    INT i;

    for (i=Len-1; i>=0; i--)
    {
        Incr = 1 << Bits[i];
        j = Code[i];
        do
        {
            Decode[j] = (UCHAR)i;
            j += Incr;
        } while (j<256);
    }
}

static VOID
GenAscTabs( struct LZW_EXP_DATA *Exp )
{
    UINT j, Incr;
    INT  i;

    for (i=255; i>=0; i--)
    {
        if (Exp->ChBits[i] <= 8)
        {
            Incr = 1 << Exp->ChBits[i];
            j = ChCodeAsc[i];
            do
            {
                Exp->ChLow[j] = (UCHAR)i;
                j += Incr;
            } while (j < 256);
        }
        else if (ChCodeAsc[i] & 0xFF)
        {
            Exp->ChLow[ChCodeAsc[i] & 0xFF] = 0xFF;
            if (ChCodeAsc[i] & 0x3F)
            {
                Exp->ChBits[i] -= 4;
                Incr= 1 << Exp->ChBits[i];
                j = ChCodeAsc[i] >> 4;
                do
                {
                    Exp->ChMid1[j] = (UCHAR)i;
                    j += Incr;
                } while (j < 256);
            }
            else
            {
                Exp->ChBits[i] -= 6;
                Incr = 1 << Exp->ChBits[i];
                j = ChCodeAsc[i] >> 6;
                do
                {
                    Exp->ChMid2[j] = (UCHAR)i;
                    j += Incr;
                } while (j<128);
            }
        }
        else
        {
            Exp->ChBits[i] -= 8;
            Incr = 1 << Exp->ChBits[i];
            j = ChCodeAsc[i] >> 8;
            do
            {
                Exp->ChHi[j] = (UCHAR)i;
                j += Incr;
            } while (j < 256);
        }
    }
}
