/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

/*
    vxsim:
    ccsimso -Wall -Werror -O3 -c uint64.c
    mv uint64.o uint64_sparc.o

    powerpc gnu:
    ccppc -D__powerpc_GNU__ -Wall -Werror -O3 -c uint64.c
    mv uint64.o uint64_ppc.o

*/

#define UDIV_NEEDS_NORMALIZATION 1
#define LIBGCC2_WORDS_BIG_ENDIAN 1

#define W_TYPE_SIZE 32

#define USItype                long
#define   Wtype                long
#define  UWtype  unsigned      long
#define  DWtype           long long
#define UDWtype  unsigned long long

#if LIBGCC2_WORDS_BIG_ENDIAN
struct DWstruct {Wtype high, low;};
#else
struct DWstruct {Wtype low, high;};
#endif

typedef union {
    struct DWstruct s;
    DWtype ll;
} DWunion;

#if (defined (__i386__) || defined (__i486__)) && W_TYPE_SIZE == 32
#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  __asm__ ("addl %5,%1\n\tadcl %3,%0"                   \
       : "=r" ((USItype) (sh)),                 \
         "=&r" ((USItype) (sl))                 \
       : "%0" ((USItype) (ah)),                 \
         "g" ((USItype) (bh)),                  \
         "%1" ((USItype) (al)),                 \
         "g" ((USItype) (bl)))
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subl %5,%1\n\tsbbl %3,%0"                   \
       : "=r" ((USItype) (sh)),                 \
         "=&r" ((USItype) (sl))                 \
       : "0" ((USItype) (ah)),                  \
         "g" ((USItype) (bh)),                  \
         "1" ((USItype) (al)),                  \
         "g" ((USItype) (bl)))
#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("mull %3"                            \
       : "=a" ((USItype) (w0)),                 \
         "=d" ((USItype) (w1))                  \
       : "%0" ((USItype) (u)),                  \
         "rm" ((USItype) (v)))
#define udiv_qrnnd(q, r, n1, n0, dv) \
  __asm__ ("divl %4"                            \
       : "=a" ((USItype) (q)),                  \
         "=d" ((USItype) (r))                   \
       : "0" ((USItype) (n0)),                  \
         "1" ((USItype) (n1)),                  \
         "rm" ((USItype) (dv)))
#define count_leading_zeros(count, x) \
  do {                                  \
    USItype __cbtmp;                            \
    __asm__ ("bsrl %1,%0"                       \
         : "=r" (__cbtmp) : "rm" ((USItype) (x)));          \
    (count) = __cbtmp ^ 31;                     \
  } while (0)
#define count_trailing_zeros(count, x) \
  __asm__ ("bsfl %1,%0" : "=r" (count) : "rm" ((USItype)(x)))
#endif /* 80x86 */

#if defined (__powerpc_GNU__)

#define __ll_B ((UWtype) 1 << (W_TYPE_SIZE / 2))
#define __ll_lowpart(t) ((UWtype) (t) & (__ll_B - 1))
#define __ll_highpart(t) ((UWtype) (t) >> (W_TYPE_SIZE / 2))

#define add_ssaaaa(sh, sl, ah, al, bh, bl) \
  do {                                  \
    if (__builtin_constant_p (bh) && (bh) == 0)             \
      __asm__ ("{a%I4|add%I4c} %1,%3,%4\n\t{aze|addze} %0,%2"       \
         : "=r" (sh), "=&r" (sl) : "r" (ah), "%r" (al), "rI" (bl));\
    else if (__builtin_constant_p (bh) && (bh) == ~(USItype) 0)     \
      __asm__ ("{a%I4|add%I4c} %1,%3,%4\n\t{ame|addme} %0,%2"       \
         : "=r" (sh), "=&r" (sl) : "r" (ah), "%r" (al), "rI" (bl));\
    else                                \
      __asm__ ("{a%I5|add%I5c} %1,%4,%5\n\t{ae|adde} %0,%2,%3"      \
         : "=r" (sh), "=&r" (sl)                    \
         : "%r" (ah), "r" (bh), "%r" (al), "rI" (bl));      \
  } while (0)
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  do {                                  \
    if (__builtin_constant_p (ah) && (ah) == 0)             \
      __asm__ ("{sf%I3|subf%I3c} %1,%4,%3\n\t{sfze|subfze} %0,%2"   \
           : "=r" (sh), "=&r" (sl) : "r" (bh), "rI" (al), "r" (bl));\
    else if (__builtin_constant_p (ah) && (ah) == ~(USItype) 0)     \
      __asm__ ("{sf%I3|subf%I3c} %1,%4,%3\n\t{sfme|subfme} %0,%2"   \
           : "=r" (sh), "=&r" (sl) : "r" (bh), "rI" (al), "r" (bl));\
    else if (__builtin_constant_p (bh) && (bh) == 0)            \
      __asm__ ("{sf%I3|subf%I3c} %1,%4,%3\n\t{ame|addme} %0,%2"     \
           : "=r" (sh), "=&r" (sl) : "r" (ah), "rI" (al), "r" (bl));\
    else if (__builtin_constant_p (bh) && (bh) == ~(USItype) 0)     \
      __asm__ ("{sf%I3|subf%I3c} %1,%4,%3\n\t{aze|addze} %0,%2"     \
           : "=r" (sh), "=&r" (sl) : "r" (ah), "rI" (al), "r" (bl));\
    else                                \
      __asm__ ("{sf%I4|subf%I4c} %1,%5,%4\n\t{sfe|subfe} %0,%3,%2"  \
           : "=r" (sh), "=&r" (sl)                  \
           : "r" (ah), "r" (bh), "rI" (al), "r" (bl));      \
  } while (0)
#define count_leading_zeros(count, x) \
  __asm__ ("{cntlz|cntlzw} %0,%1" : "=r" (count) : "r" (x))

#define udiv_qrnnd(q, r, n1, n0, d) \
  do {                                  \
    UWtype __d1, __d0, __q1, __q0;                  \
    UWtype __r1, __r0, __m;                     \
    __d1 = __ll_highpart (d);                       \
    __d0 = __ll_lowpart (d);                        \
                                    \
    __r1 = (n1) % __d1;                         \
    __q1 = (n1) / __d1;                         \
    __m = (UWtype) __q1 * __d0;                     \
    __r1 = __r1 * __ll_B | __ll_highpart (n0);              \
    if (__r1 < __m)                         \
      {                                 \
    __q1--, __r1 += (d);                        \
    if (__r1 >= (d)) /* i.e. we didn't get carry when adding to __r1 */\
      if (__r1 < __m)                       \
        __q1--, __r1 += (d);                    \
      }                                 \
    __r1 -= __m;                            \
                                    \
    __r0 = __r1 % __d1;                         \
    __q0 = __r1 / __d1;                         \
    __m = (UWtype) __q0 * __d0;                     \
    __r0 = __r0 * __ll_B | __ll_lowpart (n0);               \
    if (__r0 < __m)                         \
      {                                 \
    __q0--, __r0 += (d);                        \
    if (__r0 >= (d))                        \
      if (__r0 < __m)                       \
        __q0--, __r0 += (d);                    \
      }                                 \
    __r0 -= __m;                            \
                                    \
    (q) = (UWtype) __q1 * __ll_B | __q0;                \
    (r) = __r0;                             \
  } while (0)

#define umul_ppmm(ph, pl, m0, m1) \
  do {                                  \
    USItype __m0 = (m0), __m1 = (m1);                   \
    __asm__ ("mulhwu %0,%1,%2" : "=r" (ph) : "%r" (m0), "r" (m1));  \
    (pl) = __m0 * __m1;                         \
  } while (0)

#endif

#if defined (__sparc__)

#define count_leading_zeros(count, x) \
  do {                                  \
  __asm__ ("scan %1,1,%0"                       \
       : "=r" ((USItype) (count))                   \
       : "r" ((USItype) (x)));                  \
  } while (0)

#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("! Inlined umul_ppmm\n"                  \
"   wr  %%g0,%2,%%y ! SPARC has 0-3 delay insn after a wr\n"\
"   sra %3,31,%%o5  ! Don't move this insn\n"       \
"   and %2,%%o5,%%o5    ! Don't move this insn\n"       \
"   andcc   %%g0,0,%%g1 ! Don't move this insn\n"       \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,%3,%%g1\n"                     \
"   mulscc  %%g1,0,%%g1\n"                      \
"   add %%g1,%%o5,%0\n"                     \
"   rd  %%y,%1"                         \
       : "=r" ((USItype) (w1)),                 \
         "=r" ((USItype) (w0))                  \
       : "%rI" ((USItype) (u)),                 \
         "r" ((USItype) (v))                    \
       : "g1", "o5", "cc")

#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subcc %r4,%5,%1\n\tsubx %r2,%3,%0"              \
       : "=r" ((USItype) (sh)),                 \
         "=&r" ((USItype) (sl))                 \
       : "rJ" ((USItype) (ah)),                 \
         "rI" ((USItype) (bh)),                 \
         "rJ" ((USItype) (al)),                 \
         "rI" ((USItype) (bl))                  \
         : "cc")

#define udiv_qrnnd(__q, __r, __n1, __n0, __d) \
  __asm__ ("mov %2,%%y;nop;nop;nop;udiv %3,%4,%0;umul %0,%4,%1;sub %3,%1,%1"\
       : "=&r" ((USItype) (__q)),                   \
         "=&r" ((USItype) (__r))                    \
       : "r" ((USItype) (__n1)),                    \
         "r" ((USItype) (__n0)),                    \
         "r" ((USItype) (__d)))

#endif

static UDWtype
__udivmoddi4 (UDWtype n, UDWtype d, UDWtype *rp)
{
    DWunion ww;
    DWunion nn, dd;
    DWunion rr;
    UWtype d0, d1, n0, n1, n2;
    UWtype q0, q1;
    UWtype b, bm;

    nn.ll = n;
    dd.ll = d;

    d0 = dd.s.low;
    d1 = dd.s.high;
    n0 = nn.s.low;
    n1 = nn.s.high;

#if !UDIV_NEEDS_NORMALIZATION
    if (d1 == 0)
    {
        if (d0 > n1)
        {
            /* 0q = nn / 0D */

            udiv_qrnnd (q0, n0, n1, n0, d0);
            q1 = 0;

            /* Remainder in n0.  */
        }
        else
        {
            /* qq = NN / 0d */

            if (d0 == 0)
                d0 = 1 / d0;    /* Divide intentionally by zero.  */

            udiv_qrnnd (q1, n1, 0, n1, d0);
            udiv_qrnnd (q0, n0, n1, n0, d0);

            /* Remainder in n0.  */
        }

        if (rp != 0)
        {
            rr.s.low = n0;
            rr.s.high = 0;
            *rp = rr.ll;
        }
    }

#else /* UDIV_NEEDS_NORMALIZATION */

    if (d1 == 0)
    {
        if (d0 > n1)
        {
            /* 0q = nn / 0D */

            count_leading_zeros (bm, d0);

            if (bm != 0)
            {
                /* Normalize, i.e. make the most significant bit of the
                   denominator set.  */

                d0 = d0 << bm;
                n1 = (n1 << bm) | (n0 >> (W_TYPE_SIZE - bm));
                n0 = n0 << bm;
            }

            udiv_qrnnd (q0, n0, n1, n0, d0);
            q1 = 0;

            /* Remainder in n0 >> bm.  */
        }
        else
        {
            /* qq = NN / 0d */

            if (d0 == 0)
                d0 = 1 / d0;    /* Divide intentionally by zero.  */

            count_leading_zeros (bm, d0);

            if (bm == 0)
            {
                /* From (n1 >= d0) /\ (the most significant bit of d0 is set),
                   conclude (the most significant bit of n1 is set) /\ (the
                   leading quotient digit q1 = 1).

                   This special case is necessary, not an optimization.
                   (Shifts counts of W_TYPE_SIZE are undefined.)  */

                n1 -= d0;
                q1 = 1;
            }
            else
            {
                /* Normalize.  */

                b = W_TYPE_SIZE - bm;

                d0 = d0 << bm;
                n2 = n1 >> b;
                n1 = (n1 << bm) | (n0 >> b);
                n0 = n0 << bm;

                udiv_qrnnd (q1, n1, n2, n1, d0);
            }

            /* n1 != d0...  */

            udiv_qrnnd (q0, n0, n1, n0, d0);

            /* Remainder in n0 >> bm.  */
        }

        if (rp != 0)
        {
            rr.s.low = n0 >> bm;
            rr.s.high = 0;
            *rp = rr.ll;
        }
    }
#endif /* UDIV_NEEDS_NORMALIZATION */

    else
    {
        if (d1 > n1)
        {
            /* 00 = nn / DD */

            q0 = 0;
            q1 = 0;

            /* Remainder in n1n0.  */
            if (rp != 0)
            {
                rr.s.low = n0;
                rr.s.high = n1;
                *rp = rr.ll;
            }
        }
        else
        {
            /* 0q = NN / dd */

            count_leading_zeros (bm, d1);
            if (bm == 0)
            {
                /* From (n1 >= d1) /\ (the most significant bit of d1 is set),
                   conclude (the most significant bit of n1 is set) /\ (the
                   quotient digit q0 = 0 or 1).

                   This special case is necessary, not an optimization.  */

                /* The condition on the next line takes advantage of that
                   n1 >= d1 (true due to program flow).  */
                if (n1 > d1 || n0 >= d0)
                {
                    q0 = 1;
                    sub_ddmmss (n1, n0, n1, n0, d1, d0);
                }
                else
                    q0 = 0;

                q1 = 0;

                if (rp != 0)
                {
                    rr.s.low = n0;
                    rr.s.high = n1;
                    *rp = rr.ll;
                }
            }
            else
            {
                UWtype m1, m0;
                /* Normalize.  */

                b = W_TYPE_SIZE - bm;

                d1 = (d1 << bm) | (d0 >> b);
                d0 = d0 << bm;
                n2 = n1 >> b;
                n1 = (n1 << bm) | (n0 >> b);
                n0 = n0 << bm;

                udiv_qrnnd (q0, n1, n2, n1, d1);
                umul_ppmm (m1, m0, q0, d0);

                if (m1 > n1 || (m1 == n1 && m0 > n0))
                {
                    q0--;
                    sub_ddmmss (m1, m0, m1, m0, d1, d0);
                }

                q1 = 0;

                /* Remainder in (n1n0 - m1m0) >> bm.  */
                if (rp != 0)
                {
                    sub_ddmmss (n1, n0, n1, n0, m1, m0);
                    rr.s.low = (n1 << b) | (n0 >> bm);
                    rr.s.high = n1 >> bm;
                    *rp = rr.ll;
                }
            }
        }
    }

    ww.s.low = q0;
    ww.s.high = q1;
    return ww.ll;
}


UDWtype
__umoddi3 (UDWtype u, UDWtype v)
{
    UDWtype w;
    (void) __udivmoddi4 (u, v, &w);
    return w;
}

UDWtype
__udivdi3 (UDWtype n, UDWtype d)
{
    return __udivmoddi4 (n, d, (UDWtype *) 0);
}

#if 0

#include <stdio.h>

unsigned long long
pfk_testfunc2( unsigned long long a, unsigned long long b )
{
    return a / b;
}

/*

the following should output:
   31, 4294967276
   0, 4
   7, 4294967291

*/

void
pfk_testfunc( void )
{
    unsigned long long a, b, c;
    unsigned long * d;

    a = 137438953452ULL;
    b = 4;

    c = pfk_testfunc2( a, b );

    d = (unsigned long *)&a;
    printf( "%lu, %lu\n", d[0], d[1] );

    d = (unsigned long *)&b;
    printf( "%lu, %lu\n", d[0], d[1] );

    d = (unsigned long *)&c;
    printf( "%lu, %lu\n", d[0], d[1] );
}
#endif
