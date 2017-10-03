/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

/*
 * to manually update mmu tables with windshell, you will need to do the 
 * following:
 *
 *    lkup "write_prot"
 *       find "write_protect_mmu" function
 *    unwrite_protect_page <that address>
 *    l <that address>
 *       find what the opcode for 'blr' is
 *    m <that address>,4
 *       put a 'blr' there
 *    unwrite_protect_mmu
 *    get_sdr1
 *
 * then you can do whatever you want with the mmu tables.
 */

#define ULONG unsigned long

typedef union {
    struct {
        ULONG sr:4;
        ULONG pi:16;
        ULONG off:12;
    } f;
    ULONG d;
} EA;

typedef union {
    struct {
        ULONG rpn:20;
        ULONG off:12;
    } f;
    ULONG d;
} PA;

typedef union {
    struct {
        ULONG top7:7;
        ULONG top9:9;
        ULONG low10:10;
        ULONG zero:6;
    } f;
    ULONG d;
} PTEG;

typedef union {
    struct {
        ULONG top9:9;
        ULONG low10:10;
    } f;
    ULONG d:19;
} HASH;

typedef union {
    struct {
        ULONG org7:7;
        ULONG org9:9;
        ULONG gap:7;
        ULONG mask:9;
    } f;
    ULONG d;
} SDR1;

typedef struct {
    union {
        struct {
            ULONG v:1;             /* valid */
            ULONG vsid:24;         /* virtual segment identifier */
            ULONG h:1;             /* hash identifier */
            ULONG api:6;           /* abreviated page index */
        } f;
        ULONG d;
    } one;

    union {
        struct {
            ULONG rpn:20;          /* real page number */
            ULONG zero1:3;        /* unused spare bits */
            ULONG r:1;             /* referenced */
            ULONG c:1;             /* changed */
            ULONG w:1;             /* write-through */
            ULONG i:1;             /* cache inhibit */
            ULONG m:1;             /* memory coherence required */
            ULONG g:1;             /* guarded */
            ULONG zero2:1;        /* unused bits */
            ULONG pp:2;            /* page protection bits */
        } f;
        ULONG d;
    } two;
} MMU_PTE;

int
main( int argc, char ** argv )
{
    EA ea;
    PA pa;
    PTEG pteg;
    SDR1 sdr1;
    HASH hash;
    ULONG v, w, i, m, g, pp, voff;
    MMU_PTE pte;

    if ( argc == 12 )
    {

        sdr1.d      = strtoul( argv[1] , 0, 0 );
        sdr1.f.mask = strtoul( argv[2] , 0, 0 );
        ea.d        = strtoul( argv[3] , 0, 0 );
        pa.d        = strtoul( argv[4] , 0, 0 );
        voff        = strtoul( argv[5] , 0, 0 );
        v           = strtoul( argv[6] , 0, 0 );
        w           = strtoul( argv[7] , 0, 0 );
        i           = strtoul( argv[8] , 0, 0 );
        m           = strtoul( argv[9] , 0, 0 );
        g           = strtoul( argv[10], 0, 0 );
        pp          = strtoul( argv[11], 0, 0 );

        pte.one.f.v = v;
        pte.one.f.vsid = (ea.f.sr << 8) + voff;

        pte.one.f.api = ea.f.pi >> 10;
        pte.two.f.rpn = pa.f.rpn;

        pte.two.f.w  = w;
        pte.two.f.i  = i;
        pte.two.f.m  = m;
        pte.two.f.g  = g;
        pte.two.f.pp = pp;

        pte.two.f.zero1 = 0;
        pte.two.f.zero2 = 0;
        pteg.f.zero  = 0;

        /* primary hash */

        pte.one.f.h = 0;
        hash.d = pte.one.f.vsid ^ ea.f.pi;

        pteg.f.top7  = sdr1.f.org7;
        pteg.f.top9  = sdr1.f.org9 | ( sdr1.f.mask & hash.f.top9 );
        pteg.f.low10 = hash.f.low10;

        printf( "   pteg 0x%08x : ", pteg.d );
        pte.two.f.r = pte.two.f.c = 0;
        printf( "%08x %08x  OR  ", pte.one.d, pte.two.d );
        pte.two.f.r = pte.two.f.c = 1;
        printf( "%08x %08x\n", pte.one.d, pte.two.d );

        /* alt hash */

        hash.d ^= 0x7ffff;
        pte.one.f.h = 1;

        pteg.f.top7  = sdr1.f.org7;
        pteg.f.top9  = sdr1.f.org9 | ( sdr1.f.mask & hash.f.top9 );
        pteg.f.low10 = hash.f.low10;

        printf( "altpteg 0x%08x : ", pteg.d );
        pte.two.f.r = pte.two.f.c = 0;
        printf( "%08x %08x  OR  ", pte.one.d, pte.two.d );
        pte.two.f.r = pte.two.f.c = 1;
        printf( "%08x %08x\n", pte.one.d, pte.two.d );

    }
    else if ( argc == 6 )
    {
        sdr1.d      = strtoul( argv[1] , 0, 0 );
        sdr1.f.mask = strtoul( argv[2] , 0, 0 );
        pteg.d      = strtoul( argv[3] , 0, 0 );
        pte.one.d   = strtoul( argv[4] , 0, 0 );
        pte.two.d   = strtoul( argv[5] , 0, 0 );

        printf( "one:\n"
                "  valid = %d  vsid = %#x  h = %d  api = %#x\n",
                pte.one.f.v, pte.one.f.vsid, pte.one.f.h, pte.one.f.api
            );
        printf( "two:\n"
                "  rpn = %#x  ref = %d  chg = %d  write thru = %d\n"
                "  cache inhibit = %d  mem coh = %d  guarded = %d\n"
                "  prot = %d\n",
                pte.two.f.rpn, pte.two.f.r, pte.two.f.c, pte.two.f.w,
                pte.two.f.i, pte.two.f.m, pte.two.f.g, pte.two.f.pp
            );

        if ( sdr1.d != 0 )
        {
            pa.f.off = 0;
            pa.f.rpn = pte.two.f.rpn;
            ea.f.off = 0;
            ea.f.sr = 0; // unknowable

            hash.f.low10 = pteg.f.low10;
            hash.f.top9  = sdr1.f.mask & pteg.f.top9;

            ea.f.pi = hash.d ^ pte.one.f.vsid;

            if ( pte.one.f.h == 1 )
                ea.f.pi ^= 0xffff;

            printf( "pa = 0x%08x\n", pa.d );
            printf( "ea = 0xz%07x, where 'z' depends on SR and vsid\n", ea.d );
            printf( "vsid = %#x\n", pte.one.f.vsid );
        }

    }
    else
    {
        printf( "usage: mmu htaborg htabmask ea pa voff v w i m g pp\n" );
        printf( "   or: mmu htaborg htabmask ptegaddr pte1 pte2\n" );
        exit( 0 );
    }

    return 0;
}
