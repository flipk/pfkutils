#if 0
# /* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:4 -*- */
set -e
if [ conv.cc -nt c -o conv.h -nt c ] ; then
  g++ -O6 conv.cc -o c
fi
./c
exit 0

/* also
 g++ -O -fprofile-arcs -ftest-coverage conv.cc -o c
 ./c
 gcov conv.cc
*/
;
#endif

#define VERBOSE 0
#define TRY_PKTS 1000

#define BITERRS (encoded_bits/10)

#define ENABLE_FULL 1
#define ENABLE_LAZY 0

#define DISPLAY_NODES_PROCESSED 0

#define LONG_PACKET 1
#define TEST_CONV  CONV_a3_a4_a5

//#define SEED 1234487968

#include <inttypes.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

//#define NDEBUG
#include <assert.h>

#include "conv.h"

enum CONV_ID {
    CONV_0_1,
    CONV_1_1_2_3_3,
    CONV_1_2_3,
    CONV_1_2_3_3,
    CONV_1_2_3_1_2_3,
    CONV_4_4_5_6_6,
    CONV_4_5_6,
    CONV_4_5_6_6,
    CONV_4_7,
    CONV_4_7_5,
    CONV_a1_a2,
    CONV_a3_a4_a5,
    NUM_CONVS
};

#define SOFT_ONE       -127
#define WEAK_SOFT_ONE    -1
#define SOFT_ZERO       127
#define WEAK_SOFT_ZERO    1
#define HARD_ONE       0xff
#define HARD_ZERO      0x00

#define SOFT_TO_HARD(x) (((x) < 0) ? HARD_ONE : HARD_ZERO)
#define HARD_TO_SOFT(x) ((x) ? SOFT_ONE : SOFT_ZERO)

static struct conv_polys_t {
    int num_polys;
    int polys[6]; // largest collection
} conv_polys[NUM_CONVS] = {
    { 2, { CONV_POLY_0,  CONV_POLY_1                }},
    { 5, { CONV_POLY_1,  CONV_POLY_1,  CONV_POLY_2,
           CONV_POLY_3,  CONV_POLY_3                }},
    { 3, { CONV_POLY_1,  CONV_POLY_2,  CONV_POLY_3  }},
    { 4, { CONV_POLY_1,  CONV_POLY_2,  CONV_POLY_3,
           CONV_POLY_3                              }},
    { 6, { CONV_POLY_1,  CONV_POLY_2,  CONV_POLY_3,
           CONV_POLY_1,  CONV_POLY_2,  CONV_POLY_3  }},
    { 5, { CONV_POLY_4,  CONV_POLY_4,  CONV_POLY_5,
           CONV_POLY_6,  CONV_POLY_6                }},
    { 3, { CONV_POLY_4,  CONV_POLY_5,  CONV_POLY_6  }},
    { 4, { CONV_POLY_4,  CONV_POLY_5,  CONV_POLY_6,
           CONV_POLY_6                              }},
    { 2, { CONV_POLY_4,  CONV_POLY_7                }},
    { 3, { CONV_POLY_4,  CONV_POLY_7,  CONV_POLY_5  }},
    { 2, { CONV_POLY_a1, CONV_POLY_a2               }},
    { 3, { CONV_POLY_a3, CONV_POLY_a4, CONV_POLY_a5 }}
};

#if VERBOSE
#define PRINTF(x...) printf(x)
#else
#define PRINTF(x...) do { } while(0)
#endif

struct conv_t
{
    CONV_ID id;
    int degree;      // max degree of all polynomials
    int num_states;  // number of states, 2^degree
    int num_polys;   // also number of bits in symbol
    int state_mask;
    int half_num_states;
    int max_sym_metric;
    int * polys;     // polynomials (bit masks in order of output bits)
    int8_t * matrix;   // transition matrix for walking the trellis
    int matrix_ind(int row, int col) {
        return (row * num_polys) + col;
    }
    //
    conv_t(void) {
        degree = num_states = num_polys = 0;
        matrix = NULL;
        polys = NULL;
    }
    ~conv_t(void) {
        free();
    }
    int calc_degree (int p) { // find the degree of a polynomial
        int deg;
        for (deg = -1; p > 0; ++deg)
            p >>= 1;
        return deg;
    }
    int parity (int v) { // find the parity of an integer
        v ^= (v >> 1);
        v ^= (v >> 2);
        v = (v & 0x11111111U) * 0x11111111U;
        return (v >> 28) & 1;
    }
    void init(CONV_ID _id) { // populate polys and matrix.
        id = _id;
        degree = 0;
        num_polys = conv_polys[_id].num_polys;
        for (int i = 0; i < num_polys; ++i) {
            int _d = calc_degree (conv_polys[_id].polys[i]);
            if (_d > degree)
                degree = _d;
        }
        num_states = 1 << degree; // states = 2^degree
        state_mask = num_states-1;
        half_num_states = num_states/2;
        max_sym_metric = SOFT_ZERO * num_polys;
        polys = new int[num_polys];
        matrix = new int8_t[half_num_states * num_polys];
        memcpy (polys, conv_polys[_id].polys,
                num_polys * sizeof (int));
        int i,j;
        // the bottom most bit in all the polys is always 1.
        // so if 'state' is even, 'state+1' always inverts.
        // so optimize the matrix by only calculating the evens.
        for (i = 0; i < half_num_states; ++i)
            for (j = 0; j < num_polys; ++j) {
                int encoded_bit = parity(polys[j] & (i << 1));
                matrix[matrix_ind(i,j)] = (encoded_bit ? -1 : 1);
            }
    }
    void free(void) {
        if (matrix != NULL) {
            delete[] matrix;
            matrix = NULL;
        }
        if (polys != NULL) {
            delete[] polys;
            polys = NULL;
        }
    }
    void print(void) { // debug dump of polys and matrix.
        int ind, bit, row, col;
        printf("\n");
        printf("CONV_ID = %d\n", id);
        printf("   degree = %d  num_states = %d   num_polys = %d\n",
               degree, num_states, num_polys);
        printf("   polys : ");
        for (ind = 0; ind < num_polys; ind++)
        {
            for (bit = degree; bit >= 0; bit--)
                printf("%d", polys[ind] & (1<<bit) ? 1 : 0);
            printf(" ");
        }
        for (row = 0; row < half_num_states; row++) {
            printf("\n   matrix[");
            for (bit = degree-2; bit >= 0; bit--)
                printf("%d", row & (1<<bit) ? 1 : 0);
            printf("] : ");
            for (col = 0; col < num_polys; col++)
                printf("%3d ", matrix[matrix_ind(row,col)]);
        }
        printf("\n");
    }

    // convolutionally encode a buffer
    int encode(const int8_t *dec_bits, int8_t *enc_bits, int num_dec_bits) {
        int i,j,poly_ind,poly, bit_ind;
        for (i = 0, j = 0; i < num_dec_bits; ++i) {
            for (poly_ind = 0; poly_ind < num_polys; ++poly_ind, ++j) {
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

    // the value of matrix[bit][xyz] is the metric if xyz
    // came from 0xyz, or negated if from 1xyz.

    // for each state xyzt
    // -  there are two predecessors, 0xyz and 1xyz
    // -- these two predecessors competed with each other to
    //    give me their best metric.
    // -- both predecessors reference the same matrix[][xyz].
    //
    // -  there two successors, yzt0 and yzt1
    // -- yzt0's metric came from 0yzt and 1yzt in competition
    // -- yzt1's metric came from 1yzt and 0yzt in competition
    // -- 0yzt and 1yzt used different matrix values (0yz and 1yz).

    // const int8_t * symbol = enc_bits + (bitpos * num_polys)
    int calc_delta(const int8_t * symbol, int bitpos, int state) {
        int delta = 0;
        int8_t * matrixptr = matrix + (state/2)*num_polys;
        for (int bitind = 0; bitind < num_polys; ++bitind, symbol++, matrixptr++)
            delta += *matrixptr * *symbol;
        PRINTF("calculated delta %d at bitpos %d (m[%d]) state %d\n",
               delta, bitpos, bitpos+1, state);
        return delta;
    }

#define INVALID_METRIC 2000000000

#if VERBOSE
#define PRINTTRELLIS()                                                  \
    do {                                                                \
        int i,j;                                                        \
        printf("METS");                                                 \
        for (j = 0; j < num_states; j ++)                               \
            printf("   %3d    ",j);                                     \
        printf("\n");                                                   \
        for (i = 0; i < (num_dec_bits+1); ++i)                          \
        {                                                               \
            int match = -1;                                             \
            uint32_t min = INVALID_METRIC;                              \
            for (j = 0; j < num_states; j ++)                           \
                if (trellis[i][j].metric != -1 &&                       \
                    trellis[i][j].metric < min)                         \
                {                                                       \
                    min = trellis[i][j].metric;                         \
                    match = j;                                          \
                }                                                       \
            printf("%2d: ", i);                                         \
            for (j = 0; j < num_states; j ++)                           \
                if (trellis[i][j].pred == -1)                           \
                    printf("          ");                               \
                else                                                    \
                    printf("%5d-%-3d%c",                                \
                           trellis[i][j].metric, trellis[i][j].pred,    \
                           j == match ? '<' : ' ');                     \
            printf("\n");                                               \
        }                                                               \
    } while (0)
#else
#define PRINTTRELLIS() do {} while(0)
#endif

    // lazy viterbi decoder
    int decode_lazy(int8_t *dec_bits, const int8_t *enc_bits,
                    int num_dec_bits) {
        int nodes_processed = 0;
        struct node {
            uint32_t metric;
            int16_t delta;
            int16_t pred;
        };
        node trellis[num_dec_bits+1][num_states];

        memset(trellis,0xFF,sizeof(trellis));

        struct pqnode {
            int16_t bitpos;
            int16_t state;
            pqnode * next;
            void set(int _b, int _s) {
                bitpos = _b; state = _s;
            }
        };
        struct prioq {
            // alloc
            pqnode * store;
            int storeindex;
            pqnode * alloc(void) {
                assert(storeindex > 0);
                PRINTF("alloc index %d at 0x%x\n", storeindex, 
                       (uintptr_t) &store[storeindex]);
                return &store[storeindex--];
            }
            // pqs
            int numqs;
            pqnode ** qheads;
            int lowest_non_empty_q;
            uint32_t maxmetric;
            uint32_t min_metric;
            int circular_start;
//
            prioq(pqnode * _fs, int fssz, pqnode ** _q, int _numqs,
                  uint32_t _maxmet)
                : store(_fs), storeindex(fssz-1),
                  numqs(_numqs), qheads(_q),
                  lowest_non_empty_q(_numqs), maxmetric(_maxmet),
                  min_metric(0), circular_start(0)
            {
                memset(qheads, 0, sizeof(*_q) * numqs);
            }
            int calc_qind(int met) { return (met+circular_start)%numqs; }
            void enqueue(pqnode *n, uint32_t metric) {
                PRINTF("enqueue (%d) bitpos %d (m[%d]) state %d "
                       "metric %d\n", n-store,
                       n->bitpos, n->bitpos+1, n->state, metric);
                assert(((int)metric) >= 0);
                assert(metric >= min_metric);
                metric -= min_metric;
                if (metric > maxmetric) {
                    PRINTF("downshift minmetric %d by %d (lowest %d)\n",
                           min_metric, SOFT_ZERO*2, lowest_non_empty_q);
                    min_metric += SOFT_ZERO*2;
                    circular_start += SOFT_ZERO*2;
                    lowest_non_empty_q -= SOFT_ZERO*2;
                    metric -= SOFT_ZERO*2;
                }
                if (!(metric < numqs)) {
                    printf("enqueue (%d) bitpos %d (m[%d]) state %d "
                       "metric %d\n", n-store,
                       n->bitpos, n->bitpos+1, n->state, metric);
                    print();
                }
                assert(metric < numqs);
                int qind = calc_qind(metric);
                n->next = qheads[qind];
                qheads[qind] = n;
                if (metric < lowest_non_empty_q) {
                    lowest_non_empty_q = metric;
                }
#if VERBOSE
                print();
#endif
            }
            pqnode * dequeue(void) {
                int qind = calc_qind(lowest_non_empty_q);
                pqnode * ret = qheads[qind];
                qheads[qind] = ret->next;
                if (ret->next == 0)
                {
                    int newind;
                    for (newind = lowest_non_empty_q; newind < numqs; newind++) {
                        qind = calc_qind(newind);
                        if (qheads[qind] != 0)
                            break;
                    }
                    assert(newind < numqs);
                    lowest_non_empty_q = newind;
                }
                PRINTF("dequeue (%d) bitpos %d (m[%d]) state %d\n",
                       ret-store,
                       ret->bitpos, ret->bitpos+1, ret->state);
#if VERBOSE
                print();
#endif
                return ret;
            }
            void print(void) {
                printf("lowest %d minmetric %d circstart %d\n",
                       lowest_non_empty_q, min_metric, circular_start);
                for (int ind = 0; ind < numqs; ind++) {
                    if (qheads[ind] == 0)
                        continue;
                    printf("prioq %d: ", ind);
                    for (pqnode * n = qheads[ind]; n; n = n->next)
                        printf("(%d)%d-%d-%d ", n-store, n->bitpos,
                               n->state, n->next ? n->next-store : 0);
                    printf("\n");
                }
            }
        };

        const int storesize = num_dec_bits * num_states * 1 + 100;
        const int msm = max_sym_metric;
        const int hns = half_num_states;
        int16_t delta;
        pqnode  store[storesize];
        const int numqheads = (SOFT_ZERO * 2 * (num_polys+4));
        pqnode * pqheads[numqheads];
        prioq   pq(store, storesize, pqheads, numqheads,
                   SOFT_ZERO * 2 * (num_polys+2));
        pqnode * n;
        uint32_t metric;

        // initial state is 0
        trellis[0][0].pred = 0;
        delta = calc_delta(enc_bits, 0, 0);

        n = pq.alloc();
        metric = msm-delta;
        n->set(0,0);
        trellis[1][0].metric = metric;
        trellis[1][0].pred = 0;
        pq.enqueue(n,metric);

        n = pq.alloc();
        metric = msm+delta;
        n->set(0,1);
        trellis[1][1].metric = metric;
        trellis[1][1].pred = 0;
        pq.enqueue(n,metric);

        int bitpos, state;
        while (1) {
            n = pq.dequeue();
            state = n->state;
            bitpos = n->bitpos;
            if (bitpos == (num_dec_bits-1) && state == 0)
                break;
            if (bitpos < (num_dec_bits-1))
            {
                int bpp1 = bitpos+1;
                node * nod = &trellis[bpp1][state];
                // descendant states!
                int state0 = ((state << 1) + 0) & state_mask;
                int state1 = state0 + 1;
                const int8_t * symbol = enc_bits+(bpp1*num_polys);
                // lookup the branch metric for these two descendants.
                delta = nod->delta;
                if (delta == -1) {
                    delta = calc_delta(symbol,bpp1,state0);
                    // cache the delta for my sibling (and invert)
                    trellis[bpp1][state^hns].delta = -delta;
                    // if we came from an odd state, we need to
                    // invert the delta again.
                    if (state & hns)
                        delta = -delta;
                }

                metric = nod->metric;

                int bpp2 = bpp1+1;
                // schedule the two descendants of this node to be
                // processed.

                uint32_t metric0 = metric + msm - delta;
                node * nod0 = &trellis[bpp2][state0];
                if (nod0->metric > metric0) {
                    nod0->metric = metric0;
                    nod0->pred = state;
                    // reuse dequeued n
                    n->set(bpp1,state0);
                    pq.enqueue(n,metric0);
                    n = NULL; // force next to alloc new
                }

                uint32_t metric1 = metric + msm + delta;
                node * nod1 = &trellis[bpp2][state1];
                if (nod1->metric > metric1) {
                    nod1->metric = metric1;
                    nod1->pred = state;
                    if (!n) n = pq.alloc(); // reuse if not null
                    n->set(bpp1,state1);
                    pq.enqueue(n,metric1);
                }
            }
            nodes_processed++;
            PRINTTRELLIS();
        }
        PRINTTRELLIS();
#if DISPLAY_NODES_PROCESSED
        printf("lazy nodes processed = %d\n", nodes_processed);
#endif
        // backtrace through the trellis to reconstruct
        // the most likely decoded message.
        state = 0;
        for (int bitpos = num_dec_bits; bitpos > 0; bitpos--) {
            dec_bits[bitpos - 1] = (state & 1) ? HARD_ONE : HARD_ZERO;
            state = trellis[bitpos][state].pred;
        }
        return 1;
    }

#if VERBOSE
#define DUMP_METRICS()                                          \
    do {                                                        \
        int i,j;                                                \
        printf("METS");                                         \
        for (j = 0; j < num_states; j ++)                       \
            printf("  %3d     ",j);                             \
        printf("\n");                                           \
        for (i = 0; i < (num_dec_bits+1); ++i)                  \
        {                                                       \
            int match = -1, max = -1;                           \
            for (j = 0; j < num_states; j ++)                   \
                if (metrics[i][j] > max)                        \
                {                                               \
                    max = metrics[i][j];                        \
                    match = j;                                  \
                }                                               \
            printf("%2d: ", i);                                 \
            for (j = 0; j < num_states; j ++)                   \
                printf("%5d-%-3d%c", metrics[i][j], path[i][j], \
                       j == match ? '<' : ' ');                 \
            printf("\n");                                       \
        }                                                       \
    } while(0)
#else
#define DUMP_METRICS() do { } while (0)
#endif

    // convolutionally decode a buffer
    int decode(int8_t *dec_bits, const int8_t *enc_bits,
               int num_dec_bits) {
        int nodes_processed = 0;
        int memory_required = 0;
        const int8_t *symbol;
        int i, j, k, a, b, metric;
        int metrics[num_dec_bits + 1][num_states];
        int path[num_dec_bits + 1][num_states];
        memory_required = sizeof(metrics) + sizeof(path);
        // init all metrics to large negative values, so none of these
        // will ever get chosen as a surviving path.  note that the
        // matrix does not encode the current data bit, only the
        // history.
        for (i = 1; i < num_states; ++i) {
            metrics[0][i] = (-127 * num_polys * num_dec_bits) - 1;
            path[0][i] = 0;
        }
        // initial state is 0
        metrics[0][0] = 0;
        path[0][0] = 0;
        // compute path metrics, one path survives each state.
        // i moves forward through output 1 for each decoded output bit.
        // 'symbol' moves forward N bits for each encoded symbol,
        // where N is the number of polynomials in this code.
        // --note that the metrics result for bit[i] is in
        // metrics[i+1] and path[i+1].
        for (i = 0, symbol = enc_bits;
             i < num_dec_bits;
             ++i, symbol += num_polys)
        {
            // trellis butterfly, update two states at a time.  j
            // iterates over all states, counting by 2, so we can
            // update the metric at each box in metrics[i+1][j] and
            // [j+1]. we count by 2 because the deltas calculated at
            // [j] and [j+1] are the same so why calculate them twice.
            // j counts by 2, k counts by 1, so k=j/2, just for
            // convenience in indexing the matrix, since the matrix is
            // only encoding the history of degree-1 previous bits,
            // not including the current bit.
            for (j = 0, k = 0;
                 j < num_states;
                 ++k, j += 2)
            {
                int delta = 0, bitind;
                // matrix[x][y] encodes branch probabilities. y is the
                // polynomial. matrix[x][y] is the probability that
                // the history of previous bits for polynomial y which
                // match the value of x should xor to 0 given the
                // newest bit is 0, or to 1 given the newest bit is 1.
                // for each bit in the current symbol, perform a MAC
                // (multiply and accumulate) on all the symbol bits.
                // this should give us a metric on whether this bit is
                // a possible/correct next bit given all the
                // polynomials if this state was a correct current
                // state.
                delta = calc_delta(symbol, i, j);
                // find 2 predecessor states. note in most pictures on
                // the internet depicting states in a trellis, they
                // code the state number as shifting left to right,
                // that is, state 101 inputing a '1' becomes 110;
                // however this code encodes a state by shifting right
                // to left, so state 101 inputing a '1' becomes
                // 011. so if you're looking at state j, what are the
                // two possible predecessors?  0 concatenated with
                // j>>1, or 1 concatenated with j>>1.  since the terms
                // of the for-loop enforce k=j/2 which is equal to
                // k=j>>1, and a 1 in the high bit is equal to
                // num_states/2, or num_states>>1, then the two
                // predecessor states are k and k+(num_states>>1).
                a = k;
                b = k + half_num_states;
                // choose best transitions.  note to get to the
                // current state j, either we came from state 'a' or
                // state 'b'. those two states by necessity have
                // opposite parities, therefore the new bit we're
                // adding to go to the next state is either going to
                // help or hurt, so compare the metrics and see which
                // one it helps.  also note we're updating two states
                // at once, one were the new bit is a zero and one
                // where the new bit is one. in the case where the new
                // bit is one, then the sign of the delta is inverted
                // because of course the parity is inverted when you
                // add another one.
                PRINTF("comparing m[%d][%d]=%d+%d to m[%d][%d]=%d-%d\n",
                       i,a,metrics[i][a],delta,
                       i,b,metrics[i][b],delta);
                if (metrics[i][a] + delta > metrics[i][b] - delta)
                {
                    PRINTF("choosing path a, ");
                    metrics[i + 1][j] = metrics[i][a] + delta;
                    path[i + 1][j] = a;
                } else {
                    PRINTF("choosing path b, ");
                    metrics[i + 1][j] = metrics[i][b] - delta;
                    path[i + 1][j] = b;
                }
                PRINTF("m[%d][%d] = %d, p[%d][%d] = %d\n",
                       i+1,j,metrics[i+1][j],
                       i+1,j,path[i+1][j]);
                PRINTF("comparing m[%d][%d]=%d-%d to m[%d][%d]=%d+%d\n",
                       i,a,metrics[i][a],delta,
                       i,b,metrics[i][b],delta);
                if (metrics[i][a] - delta > metrics[i][b] + delta)
                {
                    PRINTF("choosing path a, ");
                    metrics[i + 1][j + 1] = metrics[i][a] - delta;
                    path[i + 1][j + 1] = a;
                } else {
                    PRINTF("choosing path b, ");
                    metrics[i + 1][j + 1] = metrics[i][b] + delta;
                    path[i + 1][j + 1] = b;
                }
                PRINTF("m[%d][%d] = %d, p[%d][%d] = %d\n",
                    i+1,j+1,metrics[i+1][j+1],
                    i+1,j+1,path[i+1][j+1]);
                nodes_processed += 2;
            }
        }
        DUMP_METRICS();
        // backtrace path. always assume the final state is 0,
        // because these codes always input enough 0's at the end
        // of a block to ensure a return to state 0.
        b = 0;
        for (i = num_dec_bits; i > 0; --i) {
            // bit zero of the state corresponds to the decoded
            // bit. this works because the 'state' should represent
            // the original shift register of the encoder.
            dec_bits[i - 1] = (b & 1) ? HARD_ONE : HARD_ZERO;
            b = path[i][b];
        }
        // return a final branch metric, a measure of how good the
        // input signal was. not sure actually how well this branch
        // metric actually works as a measure of bit errors.  the only
        // perfect way to measure bit errors is to re-encode the
        // decoded message and compare coded bits, but that costs a
        // fair bit of CPU.
        metric = metrics[num_dec_bits][0];
#if DISPLAY_NODES_PROCESSED
        printf("full viterbi nodes processed = %d\n", nodes_processed);
#endif
        return (127 * num_dec_bits * num_polys) - metric;
    }
};

static conv_t convs[NUM_CONVS];

void conv_init (void)
{
    for (int conv_id = 0; conv_id < NUM_CONVS; conv_id++)
        convs[conv_id].init((CONV_ID)conv_id);
}

void conv_fini (void)
{
    for (int conv_id = 0; conv_id < NUM_CONVS; conv_id++)
        convs[conv_id].free();
}

static inline uint64_t gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}




int
main()
{
    uint8_t  inbuf[] = {
#if LONG_PACKET
        0x50, 0x48, 0x49, 0x4c, 0x4c, 0x49, 0x50, 0x4b,
        0x50, 0x48, 0x49, 0x4c, 0x4c, 0x49, 0x50, 0x4b,
        0x50, 0x48, 0x49, 0x4c, 0x4c, 0x49, 0x50, 0x4b,
        0x50, 0x48, 0x49, 0x4c, 0x4c, 0x49, 0x50, 0x4b,
#endif
        0x41, 0x5a, 0x00
    };
    int8_t inbits[sizeof(inbuf)*8];
    int8_t outbits_hold[sizeof(inbuf)*8*6]; // longest code is 1/6
    int8_t outbits[sizeof(inbuf)*8*6]; // longest code is 1/6
    int result, ind;
    CONV_ID conv = TEST_CONV;

#ifdef SEED
    srandom(SEED);
#else
    int seed = getpid() * time(NULL);
    printf("seed = %d\n",seed);
    srandom(seed);
#endif

    conv_init();
//    convs[conv].print();

    for (ind = 0; ind < (int)sizeof(inbits); ind++)
        inbits[ind] = (inbuf[ind>>3] >> ((7-ind) & 7)) & 1;

    PRINTF("input data:\n");
    for (ind = 0; ind < (int)sizeof(inbits); ind++)
        PRINTF("%d", inbits[ind] > 0 ? 1 : 0);
    PRINTF("\n");

    int encoded_bits = 
        convs[conv].encode(inbits,outbits_hold,sizeof(inbits));

    PRINTF("conv_encode returns %d\n", encoded_bits);
    for (ind = 0; ind < encoded_bits; ind++)
        PRINTF("%c", outbits_hold[ind] > 0 ? '-' : '1');
    PRINTF("\n");

    int8_t outbits2[sizeof(inbits)];
    int8_t outbits3[sizeof(inbits)];

    uint64_t alg1time = 0;
    uint64_t alg2time = 0;
    int alg1badpkts = 0;
    int alg2badpkts = 0;
    int alg1errs = 0;
    int alg2errs = 0;
    int errs1, errs2;
#if VERBOSE
    int tryctr = 0;
#else
    printf("** decoding %d packets\n", TRY_PKTS);
    for (int tryctr = 0; tryctr < TRY_PKTS; tryctr++)
#endif
    {
        memcpy(outbits, outbits_hold, sizeof(outbits));

#define INVERT(pos) \
        if (outbits[pos]==-127) \
            outbits[pos]=127; \
        else \
            outbits[pos]=-127

        if (BITERRS > 0)
        {
            for (int ctr = 0; ctr < BITERRS; ctr++)
            {
                int r = random()%encoded_bits;
                INVERT(r);
            }
        }
        PRINTF("intentionally corrupted bits:\n");
        for (ind = 0; ind < encoded_bits; ind++)
            PRINTF("%c", outbits[ind] == outbits_hold[ind] ? '-' :
                   outbits[ind] > 0 ? '0' : '1');
        PRINTF("\n");

        memset(outbits2,0,sizeof(outbits2));
        memset(outbits3,0,sizeof(outbits3));

        uint64_t start_time;
        if (ENABLE_LAZY)
        {
            start_time = gettime();
            convs[conv].decode_lazy(outbits2,outbits,sizeof(inbits));
            alg1time += gettime() - start_time;
            errs1 = 0;
            for (ind = 0; ind < (int)sizeof(outbits2); ind++)
            {
                int b = (outbits2[ind] < 0 ? 1 : 0) ^ inbits[ind];
                if (b != 0)
                    errs1++;
            }
            alg1errs += errs1;
            if (errs1 > 0)
                alg1badpkts++;
        }

        if (ENABLE_FULL)
        {
            start_time = gettime();
            convs[conv].decode     (outbits3,outbits,sizeof(inbits));
            alg2time += gettime() - start_time;

            errs2 = 0;
            for (ind = 0; ind < (int)sizeof(outbits3); ind++)
            {
                int b = (outbits3[ind] < 0 ? 1 : 0) ^ inbits[ind];
                if (b != 0)
                    errs2++;
            }
            alg2errs += errs2;
            if (errs2 > 0)
                alg2badpkts++;
        }

#if !VERBOSE
        if (errs1 > 0 || errs2 > 0)
#endif
            printf("try %5d : "
#if ENABLE_LAZY
                   "errs1 = %4d  "
#endif
#if ENABLE_FULL
                   "errs2 = %4d"
#endif
                   "\n", tryctr
#if ENABLE_LAZY
                   ,errs1
#endif
#if ENABLE_FULL
                   ,errs2
#endif
                );
    }
    printf("runtime : %llu %llu (%.2f%%)\n", alg1time, alg2time, 
           (double) alg1time / (double) alg2time * (double) 100.0 );
    printf("bad pkts : %d %d (%.3f%% %.3f%%)\n", alg1badpkts, alg2badpkts,
           (double) alg1badpkts / (double) TRY_PKTS * (double) 100.0,
           (double) alg2badpkts / (double) TRY_PKTS * (double) 100.0 );
    printf("total errs : %d %d\n", alg1errs, alg2errs);
    conv_fini();

    return 0;
}
