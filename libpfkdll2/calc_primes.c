
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

#if 0
gcc -g3 calc_primes.c -o cp
exit 0
#endif

#include <stdio.h>

#define UINT32 unsigned long

#define PRIME_LIMIT   /* 134217728 */ 8388608
#define ARRAY_MEMORY  (PRIME_LIMIT/8)

#define SET_BIT(pos)  array[ pos >> 5 ] |= (1 << ( pos & 31 ))
#define GET_BIT(pos)  ((array[ pos >> 5 ] &  (1 << ( pos & 31 ))) != 0)

static inline int
next_zero( UINT32 * array, int pos )
{
    pos++;
    while ( pos < PRIME_LIMIT )
    {
        UINT32 v = array[ pos >> 5 ];
        if (( pos & 31 ) == 0 )
            if ( v == 0xFFFFFFFF )
            {
                pos += 32;
                continue;
            }
        if ( (v & (1 << (pos % 32))) == 0 )
            return pos;
        pos++;
    }
    // if we get here, we're outta bits.
    return -1;
}

int
main()
{
    UINT32  * array = (UINT32 *) malloc( ARRAY_MEMORY );
    int i, j, count;

    memset( array, 0, ARRAY_MEMORY );

/*
// the array only represents odd numbers.
// thus for any number 'n', the index in the array
//  which represents it, is "n/2".
// which means that array[0] bit 1<<0 is values 0,1
//  and array[0] bit 1<<1 is 2,3, etc.
// to convert the other way, from index to value,
//  it is value*2 or value*2+1.
//
//  suppose i know 17 is prime.
//  value = 17 but i = 8.
//  i need to clear entry for 34, 51, 68, 85, 102, 119, etc.
//  i values are: 17(even), 25, 34(even), 42, 51(even), 59.
*/

    count = 0;
    SET_BIT(0);

    // note that we skip 2 !

    i = 1;
    do {

        if ( ! GET_BIT(i) )
        {
            int val = i*2 + 1;

            printf( "%d ", val );
            count++;

            /*
            // we need to tick off all the irrelevant bits.
            // since the array only represents odd numbers,
            // and since every match is going to be odd,
            // and since every 2nd sum of an odd is going to be even,
            // we only have to mark off half as many bits.
            */

            j = i + val;
            while ( j < PRIME_LIMIT )
            {
                SET_BIT(j);
                j += val;
            }
        }

        i = next_zero( array, i );

    } while ( i < PRIME_LIMIT && i != -1 );

    fprintf( stderr, "found %d primes\n", count );

    return 0;
}
