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

// an order-40 low pass (0.5) filter using int16 samples
class fir_filter_int16_lp05_40 : public fir_filter<int16_t> {
    static const float lp05_40_coeff[41];
public:
    fir_filter_int16_lp05_40(void) : fir_filter(40,lp05_40_coeff) { }
    ~fir_filter_int16_lp05_40(void) { }
};

// an order-40 low pass (0.5) filter using float samples
class fir_filter_float_lp05_40 : public fir_filter<float> {
    static const float lp05_40_coeff[41];
public:
    fir_filter_float_lp05_40(void) : fir_filter(40,lp05_40_coeff) { }
    ~fir_filter_float_lp05_40(void) { }
};
