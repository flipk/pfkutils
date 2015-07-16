#if 0
set -e
if [ conv2.cc -nt c2 ] ; then
  g++ -O6 conv2.cc -o c2
fi
./c2
exit 0
;
#endif

#define CONVOBJ CONV_a3_a4_a5

#define DO_FULL 1
#define DO_LAZY 0

#define VERBOSE false//true

#define BITERRS 20

#define LARGE_PACKET 1
#define MEDIUM_PACKET 0

#define ITERATIONS 1500//1

#define SEED -1120491125

#define DELTA_CACHE 0

//#define NDEBUG

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "conv.h"

#define SOFT_ONE       -127
#define WEAK_SOFT_ONE    -1
#define SOFT_ZERO       127
#define WEAK_SOFT_ZERO    1
#define HARD_ONE       0xff
#define HARD_ZERO      0x00

//hardcoded to 3 polynomials
template  <bool verbose, int num_dec_bits, int degree,
           int poly1, int poly2, int poly3>
class conv3_t
{
    typedef int32_t metric_t;
    typedef int16_t  delta_t;
    typedef int16_t  state_t;
    typedef int16_t bitpos_t;
    typedef int32_t pqnode_t;
    static const int num_polys = 3;
    static const state_t num_states = 1 << degree;
    static const state_t half_num_states = num_states / 2;
    static const state_t state_mask = num_states-1;
    int8_t matrix[num_states/2][num_polys];
    int parity(int v) {
        v ^= (v >> 1);
        v ^= (v >> 2);
        v = (v & 0x11111111U) * 0x11111111U;
        return (v >> 28) & 1;
    }
    struct node {
        metric_t  metric;
#if DELTA_CACHE
        delta_t   delta;
#endif
        state_t   pred;
    };
    node trellis[num_dec_bits+1][num_states];
    static const metric_t max_sym_metric = SOFT_ZERO * num_polys;
    static const metric_t invalid_metric = 2000000000;
    int deltas;
    void trellis_init(void)
    {
        memset(trellis,0xFF,sizeof(trellis));
        deltas = 0;
    }
    void trellis_print(void)
    {
        int i,j;
        printf("METS");
        for (j = 0; j < num_states; j ++)
            printf("   %3d    ",j);
        printf("\n");
        for (i = 0; i < (num_dec_bits+1); ++i)
        {
            printf("%2d: ", i);
            for (j = 0; j < num_states; j ++)
                if (trellis[i][j].pred == -1)
                    printf("          ");
                else
                    printf("%5d-%-3d ",
                           trellis[i][j].metric, trellis[i][j].pred);
            printf("\n");
        }
    }
    struct pqnode {
        bitpos_t bitpos;
        state_t  state;
        metric_t metric;
        state_t  pred;
        pqnode_t next;
        void set(bitpos_t _b, state_t _s, metric_t _m, state_t _p) {
            bitpos = _b; state = _s; metric = _m; pred = _p;
        }
    };
    static const pqnode_t num_pqnodes = num_dec_bits * num_states * 4 + 100;
    pqnode  pqnodes[num_pqnodes];
    pqnode_t pqfreeindex;
    static const pqnode_t numqheads = 15000; //SOFT_ZERO * 2 * (num_polys+4);
    pqnode_t pqheads[numqheads];
    pqnode_t pq_lowest_notempty_q;
    void pq_init(void)
    {
        memset(pqheads,0xFF,sizeof(pqheads));
        pqfreeindex = 0;
        pq_lowest_notempty_q = numqheads;
    }
    pqnode * pq_alloc(void)
    {
        assert(pqfreeindex < num_pqnodes);
        return &pqnodes[pqfreeindex++];
    }
    void pq_enqueue(pqnode *n, metric_t metric)
    {
        if (verbose)
            printf(
                    "--> enqueue (%d) bitpos %d (m[%d]) state %d metric %d\n",
                   n - pqnodes, n->bitpos, n->bitpos+1, n->state, metric);
        assert(metric >= 0);
        n->next = pqheads[metric];
        pqheads[metric] = n - pqnodes;
        if (metric < pq_lowest_notempty_q)
            pq_lowest_notempty_q = metric;
        if (verbose)
            pq_print();
    }
    pqnode * pq_dequeue(void)
    {
        pqnode_t ret_ind = pqheads[pq_lowest_notempty_q];
        pqnode * ret = &pqnodes[ret_ind];
        pqheads[pq_lowest_notempty_q] = ret->next;
        if (ret->next == -1)
        {
            pqnode_t newind;
            for (newind = pq_lowest_notempty_q;
                 newind < numqheads;
                 newind++)
            {
                if (pqheads[newind] != -1)
                    break;
            }
            assert(newind < numqheads);
            pq_lowest_notempty_q = newind;
        }
        if (verbose)
        {
            printf("<-- dequeue (%d) bitpos %d (m[%d]) state %d "
                   "metric %d low %d \n",
                   ret_ind, ret->bitpos, ret->bitpos+1, ret->state,
                   ret->metric, pq_lowest_notempty_q);
            pq_print();
        }
        return ret;
    }
    void pq_print(void)
    {
        printf("   lowest %d\n", pq_lowest_notempty_q);
        for (pqnode_t ind = 0; ind < numqheads; ind++) {
            if (pqheads[ind] == -1)
                continue;
            printf("     prioq %d: ", ind);
            pqnode * n; pqnode_t nid;
            for (nid = pqheads[ind]; nid != -1; nid = n->next)
            {
                n = &pqnodes[nid];
                printf("(%d)%d-%d-%d ", n - pqnodes, n->bitpos,
                       n->state, n->next);
            }
            printf("\n");
        }
    }
    delta_t calc_delta(const int8_t *symbol, bitpos_t bitpos, state_t state)
    {
        int8_t * matrixptr = &matrix[state/2][0];
        delta_t delta =
            (matrixptr[0] * symbol[0]) +
            (matrixptr[1] * symbol[1]) +
            (matrixptr[2] * symbol[2]);
        if (verbose)
        {
            printf("calculated delta %d at bitpos %d (m[%d]) state %d\n",
                   delta, bitpos, bitpos+1, state);
        }
        deltas++;
        return delta;
    }
public:
    bool print_node_stats;
    conv3_t(void)
    {
        int polys[num_polys] = { poly1, poly2, poly3 };
        for (int i = 0; i < (num_states/2); i++)
            for (int j = 0; j < num_polys; j++)
            {
                int encoded_bit = parity(polys[j] & (i<<1));
                matrix[i][j] = encoded_bit ? -1 : 1;
            }
        print_node_stats = false;
    }
    ~conv3_t(void)
    {
    }
    int encode(const int8_t *dec_bits, int8_t *enc_bits)
    {
        int polys[num_polys] = { poly1, poly2, poly3 };
        int i,j,poly_ind,poly, bit_ind;
        for (i = 0, j = 0; i < num_dec_bits; ++i)
        {
            for (poly_ind = 0; poly_ind < num_polys; ++poly_ind, ++j)
            {
                int8_t hard = HARD_ZERO;
                for (poly = polys[poly_ind], bit_ind = i;
                     poly > 0 && bit_ind >= 0;
                     poly >>= 1, --bit_ind)
                {
                    int8_t polybit = (poly & 1 ? HARD_ONE : HARD_ZERO);
                    int8_t databit = (dec_bits[bit_ind] ? HARD_ONE : HARD_ZERO);
                    hard ^= (polybit & databit);
                }
                enc_bits[j] = (hard ? SOFT_ONE : SOFT_ZERO);
            }
        }
        return j;
    }
    int decode_lazy(int8_t *dec_bits, const int8_t *enc_bits)
    {
        int nodes_processed = 0;
        delta_t delta;
        metric_t metric;
        state_t state;
        bitpos_t bitpos;
        pqnode * n;

        pq_init();
        trellis_init();
        trellis[0][0].metric = 0;
        trellis[0][0].pred = 0;

        delta = calc_delta(enc_bits, 0, 0);

        metric = max_sym_metric - delta;
        n = pq_alloc();
        n->set(0,0,metric,0);
        pq_enqueue(n,metric);

        metric = max_sym_metric + delta;
        n = pq_alloc();
        n->set(0,1,metric,0);
        pq_enqueue(n,metric);

        while (1)
        {
            n = pq_dequeue();
            state = n->state;
            bitpos = n->bitpos;
            bitpos_t bpp1 = bitpos+1;
            node * nod = &trellis[bpp1][state];
            if (nod->pred != -1)
                // skip, already done
                continue;
#if DELTA_CACHE
            delta = nod->delta;
#endif
            metric = n->metric;
            nod->metric = metric;
            nod->pred = n->pred;
            if (bitpos == (num_dec_bits-1))
            {
                if (state == 0)
                    break;
                continue;
            }
            state_t state0 = ((state << 1) + 0) & state_mask;
            state_t state1 = state0 + 1;
            const int8_t * symbol = enc_bits + (bpp1 * num_polys);
#if DELTA_CACHE
            if (delta == -1) {
                delta = calc_delta(symbol, bpp1, state0);
                trellis[bpp1][state ^ half_num_states].delta = delta;
            }
#else
            delta = calc_delta(symbol, bpp1, state0);
#endif
            if (state & half_num_states)
                delta = -delta;
            bitpos_t bpp2 = bitpos+2;
            metric_t metric0 = metric + max_sym_metric - delta;
            metric_t metric1 = metric + max_sym_metric + delta;
            // reuse n
            n->set(bpp1,state0,metric0,state);
            pq_enqueue(n,metric0);

            n = pq_alloc();
            n->set(bpp1,state1,metric1,state);
            pq_enqueue(n,metric1);

            nodes_processed++;
            if (verbose)
                trellis_print();
        }
        if (verbose)
            trellis_print();
        // backtrace through the trellis to reconstruct
        // the most likely decoded message.
        state = 0;
        for (bitpos = num_dec_bits; bitpos > 0; bitpos--) {
            dec_bits[bitpos - 1] = (state & 1) ? HARD_ONE : HARD_ZERO;
            state = trellis[bitpos][state].pred;
        }
        if (print_node_stats)
        {
            printf("lazy nodes processed = %d\n", nodes_processed);
            printf("lazy deltas calculated = %d\n", deltas);
        }
        return num_dec_bits;
    }
    int decode_full(int8_t *dec_bits, const int8_t *enc_bits)
    {
        int nodes_processed = 0;
        bitpos_t bitpos;
        state_t state, state0, state1;
        delta_t delta;
        metric_t metric0, metric1;
        const int8_t *symbol;
        trellis_init();
        trellis[0][0].metric = 0;
        trellis[0][0].pred = 0;
        for (bitpos = 0, symbol = enc_bits;
             bitpos < num_dec_bits;
             bitpos++, symbol += num_polys)
        {
            for (state = 0; state < num_states; state += 2)
            {
                delta = calc_delta(symbol, bitpos, state);
                // predecessors
                state0 = state / 2;
                state1 = state0 + half_num_states;
                metric0 = trellis[bitpos][state0].metric;
                metric1 = trellis[bitpos][state1].metric;
                if (metric0 + delta > metric1 - delta)
                {
                    trellis[bitpos+1][state].metric = metric0 + delta;
                    trellis[bitpos+1][state].pred = state0;
                }
                else
                {
                    trellis[bitpos+1][state].metric = metric1 - delta;
                    trellis[bitpos+1][state].pred = state1;
                }
                if (metric0 - delta > metric1 + delta)
                {
                    trellis[bitpos+1][state+1].metric = metric0 - delta;
                    trellis[bitpos+1][state+1].pred = state0;
                }
                else
                {
                    trellis[bitpos+1][state+1].metric = metric1 + delta;
                    trellis[bitpos+1][state+1].pred = state1;
                }
                nodes_processed += 2;
            }
        }
        if (verbose)
            trellis_print();
        state = 0;
        for (bitpos = num_dec_bits;  bitpos > 0; bitpos--) {
            dec_bits[bitpos - 1] = (state & 1) ? HARD_ONE : HARD_ZERO;
            state = trellis[bitpos][state].pred;
        }
        if (print_node_stats)
        {
            printf("full nodes processed = %d\n", nodes_processed);
            printf("full deltas calculated = %d\n", deltas);
        }
        return num_dec_bits;
    }
};




static inline uint64_t gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}




uint8_t  inbuf[] = {
#if LARGE_PACKET
    0x50, 0x48, 0x49, 0x4c, 0x4c, 0x49, 0x50, 0x4b,
    0x50, 0x48, 0x49, 0x4c, 0x4c, 0x49, 0x50, 0x4b,
    0x50, 0x48, 0x49, 0x4c, 0x4c, 0x49, 0x50, 0x4b,
    0x50, 0x48, 0x49, 0x4c, 0x4c, 0x49, 0x50, 0x4b,
#endif
#if MEDIUM_PACKET
    0x50, 0x48, 0x49, 0x4c, 0x4c, 0x49, 0x50, 0x4b,
#endif
    0x5a, 0x00
};

conv3_t<VERBOSE,sizeof(inbuf)*8,8,
        CONV_POLY_a3,CONV_POLY_a4,CONV_POLY_a5>  CONV_a3_a4_a5;

int
main()
{
    int ind;
    uint64_t start_time;
    uint64_t total_time1, total_time2, this_time;
    int decoded_bits, errs1, errs2, total_errs1, total_errs2;
    int8_t inbits[sizeof(inbuf)*8];
    int8_t outbits_hold[sizeof(inbuf)*8*6]; // longest code is 1/6
    int8_t outbits[sizeof(outbits_hold)];
    int8_t outbits2[sizeof(inbits)];

#ifdef SEED
    srandom(SEED);
#else
    int seed = getpid() * time(NULL);
    printf("seed = %d\n", seed);
    srandom(seed);
#endif

    for (ind = 0; ind < (int)sizeof(inbits); ind++)
        inbits[ind] = (inbuf[ind>>3] >> ((7-ind) & 7)) & 1;

    int encoded_bits = CONVOBJ.encode(inbits,outbits_hold);

#if VERBOSE
    printf("encode returned %d\n", encoded_bits);
    for (ind = 0; ind < encoded_bits; ind++)
        printf("%c", outbits_hold[ind] > 0 ? '-' : '1');
    printf("\n");
#endif

    total_time1 = total_time2 = 0;
    total_errs1 = total_errs2 = 0;

    if (ITERATIONS == 1)
        CONVOBJ.print_node_stats = true;
    else
        printf("** doing %d decodes\n", ITERATIONS);

    for (int iter = 0; iter < ITERATIONS; iter++)
    {
        memcpy(outbits, outbits_hold, sizeof(outbits));

#define INVERT(pos)                             \
        if (outbits[pos]==-127)                 \
            outbits[pos]=127;                   \
        else                                    \
            outbits[pos]=-127

        if (BITERRS > 0)
        {
            for (int ctr = 0; ctr < BITERRS; ctr++)
            {
                int r = random()%encoded_bits;
                INVERT(r);
            }
        }
        if (ITERATIONS == 1)
        {
            printf("intentionally corrupted bits:\n");
            for (ind = 0; ind < encoded_bits; ind++)
                printf("%c", outbits[ind] == outbits_hold[ind] ? '-' :
                        outbits[ind] > 0 ? '0' : '1');
            printf("\n");
        }

#if DO_FULL
        start_time = gettime();
        decoded_bits = CONVOBJ.decode_full(outbits2,outbits);
        this_time = gettime() - start_time;
        total_time1 += this_time;
        if (ITERATIONS == 1)
        {
            printf("decode full took %d ticks\n", this_time);
//            printf("decode full returned %d\n", decoded_bits);
        }
        errs1 = 0;
        for (ind = 0; ind < (int)sizeof(outbits2); ind++)
        {
            int b = (outbits2[ind] < 0 ? 1 : 0) ^ inbits[ind];
            if (b != 0)
                errs1++;
        }
        if (ITERATIONS == 1)
            printf("%d errors\n", errs1);
        total_errs1 += errs1;
#endif

#if DO_LAZY
        start_time = gettime();
        decoded_bits = CONVOBJ.decode_lazy(outbits2,outbits);
        this_time = gettime() - start_time;
        total_time2 += this_time;
        if (ITERATIONS == 1)
        {
            printf("decode lazy took %d ticks\n", this_time);
//            printf("decode lazy returned %d\n", decoded_bits);
        }
        errs2 = 0;
        for (ind = 0; ind < (int)sizeof(outbits2); ind++)
        {
            int b = (outbits2[ind] < 0 ? 1 : 0) ^ inbits[ind];
            if (b != 0)
                errs2++;
        }
        if (ITERATIONS == 1)
            printf("%d errors\n", errs2);
        total_errs2 += errs2;
#endif
    }

#if !VERBOSE
    printf( "total time :  full=%lld lazy=%lld  (%f)\n",
            total_time1, total_time2,
            (float) total_time2 / (float) total_time1 );
    printf( "total errs : %d %d\n",
            total_errs1, total_errs2);
#endif

    return 0;
}
