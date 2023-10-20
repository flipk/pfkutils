/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <inttypes.h>

// etsi ts 125.213 v13.0.0 5.2.2 (2016-01)
//
// x17 = x7 + x0
// y17 = y10 + y7 + y5 + y0
// i = x0 + y0
// q = (x15 + x6 + x4) +
//     (y15 + y14 + y13 + y12 + y11 + y10 + y9 + y8 + y6 + y5)

struct GoldCode {
    signed char *outputs_i;
    signed char *outputs_q;
    static const int FEEDBACK_BIT = 17;
    static const int length =  262144;
    uint32_t bit(uint32_t v, int bit) {
        return ((v >> bit) & 1);
    }
    uint32_t makebit(uint32_t v, int bit) {
        return ((v & 1) << bit);
    }
    GoldCode(uint32_t seed1, uint32_t seed2) {
        outputs_i = new signed char[length];
        outputs_q = new signed char[length];
        uint32_t  x = seed1 & 0x3FFFF, y = seed2 & 0x3FFFF;
        for (int count = 0; count < length; count++) {
            uint32_t x2 = x >> 1;
            uint32_t y2 = y >> 1;
            // 5.2.2 figure 10
            uint32_t i = bit(x,0) ^ bit(y,0);
            uint32_t q =
                bit(x,15) ^ bit(x, 6) ^ bit(x, 4) ^
                bit(y,15) ^ bit(y,14) ^ bit(y,13) ^
                bit(y,12) ^ bit(y,11) ^ bit(y,10) ^
                bit(y, 9) ^ bit(y, 8) ^ bit(y, 6) ^
                bit(y, 5);
            outputs_i[count] = i ? 1 : -1;
            outputs_q[count] = q ? 1 : -1;
            x = x2 | makebit(bit(x, 7) ^ bit(x,0), FEEDBACK_BIT);
            y = y2 | makebit(bit(y,10) ^ bit(y,7) ^
                             bit(y, 5) ^ bit(y,0), FEEDBACK_BIT);
        }
    }
    ~GoldCode(void) {
        delete[] outputs_i;
        delete[] outputs_q;
    }
};
