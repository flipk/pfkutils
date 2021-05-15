/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __BINARY_IQ_FILE_H__
#define __BINARY_IQ_FILE_H__ 1

#include <inttypes.h>
#include <complex>
#include <stdio.h>

#ifndef CPLX_DOUBLE_DEFINED
#define CPLX_DOUBLE_DEFINED
typedef std::complex<double> cplx_double;
typedef std::complex<float>  cplx_float;
#endif

// 16 bit or 12 bit
// i first or q first
// little endian or big endian
// timestamps or not (and how often)
// 2s complement or offset binary (and what offset)

struct binary_iq_file_config
{
    enum { BITS_16, BITS_12 } bits;
    enum { I_FIRST, Q_FIRST } first;
    enum { LITTLE, BIG } endian;
    enum { TS, NOTS } ts;
    enum { COMPL2, BINARYOFF } rep;
    int ts_interval; // if ts==TS, iq sample count
    int binary_offset; // if rep == BINARYOFF
};

class binary_iq_file_reader
{
    uint32_t timestamp;
    binary_iq_file_config cfg;
    FILE * f;
    bool _ok;
public:
    binary_iq_file_reader(const char *fname,
                          const binary_iq_file_config &_cfg)
        : cfg(_cfg), _ok(false)
    {
        timestamp = 0;
        f = fopen(fname, "r");
        if (f)
        {
            _ok = true;
        }
    }
    virtual ~binary_iq_file_reader(void)
    {
        if (f)
            fclose(f);
    }
    bool ok(void) const { return _ok; }
    uint32_t get_timestamp(void) const { return timestamp; }
    // returns false at end of file
    bool get_sample(cplx_double &sample)
    {
        // xxx return true;
    }
};

class binary_iq_file_writer
{
    uint32_t timestamp;
    binary_iq_file_config cfg;
    FILE * f;
    bool _ok;
public:
    binary_iq_file_writer(const char *fname,
                          const binary_iq_file_config &_cfg)
        : cfg(_cfg), _ok(false)
    {
        timestamp = 0;
        f = fopen(fname, "w");
        if (f)
        {
            _ok = true;
        }
    }
    ~binary_iq_file_writer(void)
    {
        if (f)
            fclose(f);
    }
    bool ok(void) const { return _ok; }
    uint32_t get_timestamp(void) const { return timestamp; }
    void put_sample(const cplx_double &sample)
    {
        int16_t si = sample.real();
        int16_t sq = sample.imag();
        uint16_t ui, uq;
        if (cfg.rep == binary_iq_file_config::BINARYOFF)
        {
            ui = (uint16_t) si + cfg.binary_offset;
            uq = (uint16_t) sq + cfg.binary_offset;
        }
        else
        {
            ui = (uint16_t) si;
            uq = (uint16_t) sq;
        }
        uint16_t first, second;
        if (cfg.first == binary_iq_file_config::I_FIRST)
            first = ui, second = uq;
        else
            first = uq, second = ui;
        uint32_t out;
        if (cfg.bits == binary_iq_file_config::BITS_16)
            out = 
    }
};

#endif /* __BINARY_IQ_FILE_H__ */
