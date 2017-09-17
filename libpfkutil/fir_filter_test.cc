
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

#include "fir_filter.h"

#define SAMPLES 250
int16_t  input[SAMPLES];
int16_t output[SAMPLES];

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
        input[ind]  = (int16_t)(sin(angle1) * 5000);
        angle1 += FREQ_1_RADIANS_PER_SAMPLE;

        input[ind] += (int16_t)(sin(angle2) * 5000);
        angle2 += FREQ_2_RADIANS_PER_SAMPLE;
    }

    fir_filter_int16_lp05_40  filt05;

    for (int ind = 0; ind < SAMPLES; ind++)
    {
        output[ind] = filt05.one_sample(input[ind]);
    }

    // plot
    FILE * o = fopen("output.txt", "w");
    for (int ind = 0; ind < SAMPLES; ind++)
    {
        fprintf(o,"%d %d\n", input[ind], output[ind]);
    }

    return 0;
}
