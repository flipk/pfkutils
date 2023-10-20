/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __FIR_FILTER_H__
#define __FIR_FILTER_H__

#include <inttypes.h>
#include <complex>

typedef std::complex<double> cplx_double;
typedef std::complex<float>  cplx_float;

// possible optimization: if you know the filter coefficients
// are symmetric (which they are if the filter is linear) then
// you can speed up a FIR by saving a history of products.

// make a history array [order/2] [order+1].

// break the calc into three parts:
//   the first order/2, in which products are saved in the history
//   the middle sample
//   the second order/2, in which old products are taken from history.

// fill it diagonally, like a forward slash /.
// consume it reverse diagonally, like a reverse slash \.
// both slashes move one column to the right (circularly) as
// samples are added.

// note this is incompatible with sparse consumption (i.e. in the
// downsampler case where there are many add_samples but few calcs).
// this could be mitigated with a cache status bit, where a box
// is calculated if never calculated before but reused if already
// populated. this could get difficult to manage.

// todo: change all float to double

// todo: move order into the template defn and remove all the runtime
//       memory references as well as shift_register pointer refs.

template <typename _SampleType>
class fir_filter {
public:
    typedef _SampleType SampleType;
private:
    int order_p_1; // order plus one
    const float * coeff; // dimension order+1
    SampleType * shift_register; // dimension order+1
    int sr_pos; // next position to be written
public:
    fir_filter(int _order, const float * _coeff)
    {
        order_p_1 = _order + 1;
        coeff = _coeff;
        shift_register = new SampleType[order_p_1];
        for (int ind = 0; ind < order_p_1; ind++)
            shift_register[ind] = 0;
        sr_pos = 0;
    }
    virtual ~fir_filter(void)
    {
        delete[] shift_register;
    }
    void add_sample(SampleType v)
    {
        // overwrite oldest sample with newest
        shift_register[sr_pos] = v;
        if (++sr_pos >= order_p_1)
            sr_pos = 0;
    }
    SampleType calc(void)
    {
        float sum = 0;
        int sr_ind = sr_pos;
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

template <class firclass>
class cplx_fir_filter {
    firclass fir_i;
    firclass fir_q;
public:
    typedef std::complex<typename firclass::SampleType> SampleType;
    cplx_fir_filter(void) { }
    virtual ~cplx_fir_filter(void) { }
    void add_sample(SampleType v)
    {
        fir_i.add_sample(v.real());
        fir_q.add_sample(v.imag());
    }
    void calc(SampleType &out)
    {
        out.real(fir_i.calc());
        out.imag(fir_q.calc());
    }
};

// sample instantiation:
// FILTERCLASS(double,05,40);
// FILTERCLASS_COEFF(double,05,40) = {
//  -7.82233e-05,-0.00143531,0.000100237,0.00243755,  [etc]
// };
// cplx_fir_filter<fir_filter_double_lp05_40>  filt05;
// cplx_double sample(1,0);
// filt05.add_sample(sample);
// filt05.calc(sample);

#endif /* __FIR_FILTER_H__ */
