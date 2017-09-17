/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <inttypes.h>

template <typename SampleType>
class fir_filter {
    int order_p_1; // order plus one
    const float * coeff; // dimension order+1
    SampleType * shift_register; // dimension order+1
    int sr_pos; // next position to be written
public:
    fir_filter(int _order, const float * _coeff) {
        order_p_1 = _order + 1;
        coeff = _coeff;
        shift_register = new SampleType[order_p_1];
        for (int ind = 0; ind < order_p_1; ind++)
            shift_register[ind] = 0;
        sr_pos = 0;
    }
    virtual ~fir_filter(void) {
        delete[] shift_register;
    }
    void one_sample_nocalc(SampleType v) {
        // overwrite oldest sample with newest
        shift_register[sr_pos] = v;
        if (++sr_pos >= order_p_1)
            sr_pos = 0;
    }
    SampleType one_sample(SampleType v) {
        // overwrite oldest sample with newest
        shift_register[sr_pos] = v;
        float sum = 0;
        int sr_ind = sr_pos;
        if (++sr_pos >= order_p_1)
            sr_pos = 0;
        for (int ind = 0; ind < order_p_1; ind++)
        {
            sum += (float)(shift_register[sr_ind]) * coeff[ind];
            if (++sr_ind >= order_p_1)
                sr_ind = 0;
        }
        return (SampleType) sum;
    }
};

#define FILTERCLASS(sampleType,pass,order)                      \
    class fir_filter_##sampleType##_lp##pass##_##order          \
        : public fir_filter<sampleType> {                       \
        static const float coeff[order + 1];                    \
    public:                                                     \
        fir_filter_##sampleType##_lp##pass##_##order(void)      \
            : fir_filter<sampleType>(order,coeff) { }       \
        ~fir_filter_##sampleType##_lp##pass##_##order(void) { } \
    }

#define FILTERCLASS_COEFF(sampleType,pass,order) \
    const float fir_filter_##sampleType##_lp##pass##_##order::coeff[order+1]

// order-40 low pass (0.5) filter
FILTERCLASS(int16_t,05,40);

// order-40 low pass (60/325=0.18462)
FILTERCLASS(int16_t,018,40);

// order-40 low pass (541667/1000000=0.5416)
FILTERCLASS(int16_t,05416,40);

// order-40 low pass (541667/2000000=0.270833)
FILTERCLASS(int16_t,02708,40);
