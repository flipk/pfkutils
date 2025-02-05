
#include "window.h"
#include <math.h>
#include <stdio.h>

static bool is_power_of_two(int n)
{
  return (n > 0) && ((n & (n - 1)) == 0);
}

FFTWindow :: FFTWindow(WindowType  type, int N)
{
//    const char * windowname = "";

    if (!is_power_of_two(N))
    {
        printf("ERROR %d is not a power of 2!\n", N);
        exit(1);
    }
    w.resize(N);
    switch (type)
    {
    case NONE:
//        windowname = "rectangular";
        for (int ind = 0; ind < N; ind++)
            w[ind] = 1.0;
        break;
    case HAMMING:
//        windowname = "hamming";
        make_hamming(N);
        break;
    case HANN:
//        windowname = "hann";
        make_hann(N);
        break;
    case FLATTOP:
//        windowname = "flat-top";
        make_flattop(N);
        break;
    }

//    printf("constructed windowing function: %s\n", windowname);

    // now normalize using the mean.
    // this is the ACF (Amplitude Correction Factor).
    // could also choose ECF (Energy Correction Factor),
    // which requires calculating the RMS rather than mean.
    double mean = 0.0;
    for (int n = 0; n < N; n++)
        mean += w[n];
    mean /= N;
//    printf("normalizing window with factor %f\n", 1.0 / mean);
    for (int n = 0; n < N; n++)
        w[n] /= mean;
}

FFTWindow :: ~FFTWindow(void)
{
}

// https://en.wikipedia.org/wiki/Window_function#Hann_and_Hamming_windows
void FFTWindow :: make_hamming(int N)
{
    double a0 = 25.0 / 46.0; // hamming window
    for (int n = 0; n < N; n++)
        w[n] = a0 - (1.0-a0)*cos(2*M_PI*n/N);
}

void FFTWindow :: make_hann(int N)
{
    double a0 = 0.5; // hann window
    for (int n = 0; n < N; n++)
        w[n] = a0 - (1.0-a0)*cos(2*M_PI*n/N);
}

// https://en.wikipedia.org/wiki/Window_function#Flat_top_window
void FFTWindow :: make_flattop(int N)
{
    double a0 = 0.21557895;
    double a1 = 0.41663158;
    double a2 = 0.277263158;
    double a3 = 0.083578947;
    double a4 = 0.006947368;

    for (int n = 0; n < N; n++)
        w[n] = a0
            - a1*cos(2*M_PI*n/N) + a2*cos(4*M_PI*n/N)
            - a3*cos(6*M_PI*n/N) + a4*cos(8*M_PI*n/N);
}

// maybe someday
// https://en.wikipedia.org/wiki/Window_function#Dolph%E2%80%93Chebyshev_window
