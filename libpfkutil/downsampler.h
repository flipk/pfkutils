/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __DOWNSAMPLER_H__
#define __DOWNSAMPLER_H__

#include "fir_filter.h"

// to fractionally upsample, first integer upsample to something
// above the target rate, then fractionally downsample using this
// class from the integer upsampled rate to the target rate.
// (and provide a FIR filter which filters from the higher rate
//  to the target rate, to smooth over the zeros you injected
//  during the integer upsampling (also multiply the valid samples
//  by the integer upsampling rate to account for the lower average
//  due to the zeros).)

// todo: a multi-stage downsampler.
//   in this technique, a first stage downsampler uses a cheap
//   FIR filter which lets a little more through, which the
//   second stage will filter out better.

template <class firclass>
class downsampler {
public:
    typedef typename firclass::SampleType SampleType;
private:
    firclass fir;
    double accum;
    double input_incrementer;
    double output_decrementer;
    SampleType last_sample;
    bool last_sample_valid;
public:
    downsampler(double inrate, double outrate)
    {
        input_incrementer = outrate;
        output_decrementer = inrate;
        accum = output_decrementer;
        last_sample = 0;
        last_sample_valid = false;
    }
    virtual ~downsampler(void)
    {
    }
    // returns true if there's a new out sample, false if not.
    bool add_sample(SampleType _in, SampleType &out)
    {
        bool ret = false;
        double new_accum = accum + input_incrementer;
        if (new_accum >= output_decrementer)
        {
            double diff_right = new_accum - output_decrementer;
            double diff_left  = output_decrementer - accum;
            double ratio = diff_left / input_incrementer;
            // for outrate greater than 1/2 inrate, we need this
            // boolean to prevent calculating the fir math on the
            // same sample more than once and wasting cpu time.
            if (last_sample_valid == false)
                last_sample = fir.calc();
            fir.add_sample(_in);
            SampleType in = fir.calc();
            out = last_sample + (in - last_sample) * ratio;
            ret = true;
            accum = diff_right;
            last_sample = in;
            last_sample_valid = true;
        }
        else
        {
            // for outrate less than 1/2 inrate, this is never
            // true, so we are saving CPU time by not calculating
            // the fir math on samples even when we don't consume
            // the sample we just calculated.
            fir.add_sample(_in);
            last_sample_valid = false;
            accum = new_accum;
        }
        return ret;
    }
};

template <class firclass>
class cplx_downsampler {
public:
    typedef typename firclass::SampleType SampleType;
    typedef std::complex<SampleType> CplxSampleType;
private:
    downsampler<firclass> ds_r;
    downsampler<firclass> ds_i;
public:
    cplx_downsampler(double inrate, double outrate)
        : ds_r(inrate,outrate), ds_i(inrate,outrate)
    {
    }
    virtual ~cplx_downsampler(void)
    {
    }
    // returns true if 'out' contains an output sample
    bool add_sample(const CplxSampleType &in, CplxSampleType &out)
    {
        SampleType out_r, out_i;
        ds_r.add_sample(in.real(), out_r);
        bool ret = ds_i.add_sample(in.imag(), out_i);
        if (ret) {
            out.real(out_r);
            out.imag(out_i);
        }
        return ret;
    }
};

#endif /* __DOWNSAMPLER_H__ */
