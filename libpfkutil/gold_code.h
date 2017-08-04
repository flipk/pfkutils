/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <inttypes.h>


// etsi ts 125.213 v13.0.0 5.2.2 (2016-01)
//
// x17 = x7 + x0
// y17 = y10 + y7 + y5 + y0
// i = x0 + y0
// q = (x15 + x6 + x4) + (y15 + y14 + y13 + y12 + y11 + y10 + y9 + y8 + y6 + y5)

struct GoldCode {
    static const int length = 9414154;
    GoldCode(uint32_t seed1, uint32_t seed2) {
        outputs = new signed char[length];
        uint32_t  x = seed1, y = seed2;
        for (int count = 0; count < length; count++) {
            uint32_t x2 = x >> 1;
            uint32_t y2 = y >> 1;
            uint32_t i = (x & 1) ^ (y & 1);
            outputs[count] = i ? 1 : -1;
            if (((x & 0x80) >> 7) ^ (x & 1))
                x = x2 | 0x10000;
            else
                x = x2;
            if (((y & 0x400) >> 10) ^ ((y & 0x080) >>  7) ^
                ((y & 0x020) >>  5) ^ ((y & 0x001) >>  0))
                y = y2 + 0x10000;
            else
                y = y2;
        }
    }
    ~GoldCode(void) {
        delete[] outputs;
    }
    signed char *outputs;
    signed char operator[](int index) const
    {
        if (index < 0 || index >= length)
            return 0;
        return outputs[index];
    }
    static int correlate(signed char * data1, signed char * data2, int len)
    {
        int sum = 0;
        for (int pos = 0; pos < len; pos++)
            sum += (data1[pos] * data2[pos]);
        return sum;
    }
};
