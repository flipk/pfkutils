/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __FFTWINDOW_H__
#define __FFTWINDOW_H__ 1

#include <vector>

class FFTWindow {
    void make_hamming(int size);
    void make_hann(int size);
    void make_flattop(int size);
public:
    typedef enum { NONE, HAMMING, HANN, FLATTOP } WindowType;

    FFTWindow(WindowType  type, int size);
    ~FFTWindow(void);

    std::vector<double>  w;
};

#endif // __FFTWINDOW_H__
