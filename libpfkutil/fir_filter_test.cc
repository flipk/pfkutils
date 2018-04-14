
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

#include "fir_filter.h"

// pkg load signal
// format none
// f1=fir1(40,1/2)
FILTERCLASS(double,05,40);
FILTERCLASS_COEFF(double,05,40) = {
-7.82233e-05,-0.00143531,0.000100237,0.00243755,-0.000164124,-0.00455924,0.000263631,0.00811458,-0.000389016,-0.0135557,0.000528007,0.0216654,-0.000666998,-0.0340893,0.000792384,0.0551497,-0.00089189,-0.100908,0.000955777,0.316904,0.499651,0.316904,0.000955777,-0.100908,-0.00089189,0.0551497,0.000792384,-0.0340893,-0.000666998,0.0216654,0.000528007,-0.0135557,-0.000389016,0.00811458,0.000263631,-0.00455924,-0.000164124,0.00243755,0.000100237,-0.00143531,-7.82233e-05
};

#define SAMPLES 250
cplx_double     input[SAMPLES];
cplx_double    output[SAMPLES];

#define OF_NYQUIST(x)  (M_PI * (x) / 100)

#define FREQ_1_RADIANS_PER_SAMPLE  OF_NYQUIST(60)
#define FREQ_2_RADIANS_PER_SAMPLE  OF_NYQUIST(25)

int
main()
{
    int ind;
    float angle1 = 0, angle2 = 0;

    for (int ind = 0; ind < SAMPLES; ind++)
    {
        input[ind].real(cos(angle1) * 5000);
        input[ind].imag(sin(angle1) * 5000);
        angle1 += FREQ_1_RADIANS_PER_SAMPLE;

        cplx_double z(cos(angle2) * 5000,
                      sin(angle2) * 5000);
        input[ind] += z;
        angle2 += FREQ_2_RADIANS_PER_SAMPLE;
    }

    cplx_fir_filter<fir_filter_double_lp05_40>  filt05;

    for (int ind = 0; ind < SAMPLES; ind++)
    {
        filt05.add_sample(input[ind]);
        filt05.calc(output[ind]);
    }

    // plot
    FILE * i = fopen("input.txt", "w");
    FILE * o = fopen("output.txt", "w");
    for (int ind = 0; ind < SAMPLES; ind++)
    {
        fprintf(i,"%d %f %f\n", ind,  input[ind].real(),  input[ind].imag());
        fprintf(o,"%d %f %f\n", ind, output[ind].real(), output[ind].imag());
    }
    fclose(i);
    fclose(o);

    return 0;
}
