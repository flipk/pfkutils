
#include <stdio.h>
#include "averaging_algorithms.h"

#define DIM(x) (sizeof(x) / sizeof(x[0]))

void test_dg(void)
{
    double inputs[] = {
        -30, -30, -30, -30, -1, -30, -30, -30, -30, -30
    };
    Deglitcher dg;
    AverageAlg * aa = &dg;
    printf("testing deglitch(3):\n");

    dg.configure(3);
    for (int i = 0; i < DIM(inputs); i++)
    {
        double in = inputs[i];
        double out = aa->add_reading(in);
        printf("valid = %d in = %5.1f out = %5.1f\n",
               aa->valid(), in, out);
    }
    printf("\n");
}

void test_sma(void)
{
    double inputs[] = {
        -30, -30, -30, -30, -30, -30, -30, -30, -30, -30,
        -20, -20, -20, -20, -20, -20, -20, -20, -20, -20,
    };
    AverageAlgSMA sma;
    AverageAlg * aa = &sma;
    printf("testing sma(10):\n");

    sma.configure(10);
    for (int i = 0; i < DIM(inputs); i++)
    {
        double in = inputs[i];
        double out = aa->add_reading(in);
        printf("valid = %d in = %5.1f out = %5.1f\n",
               aa->valid(), in, out);
    }
    printf("\n");
}

void test_ema(void)
{
    double inputs[] = {
        -30, -20, -20, -20, -20, -20, -20, -20, -20, -20,
        -20, -20, -20, -20, -20, -20, -20, -20, -20, -20,
    };
    AverageAlgEMA ema;
    AverageAlg * aa = &ema;
    double N = 28;
    printf("testing ema(%f):\n", N);

    double alpha = AverageAlgEMA::calc_alpha(N);
    ema.configure(alpha);
    for (int i = 0; i < DIM(inputs); i++)
    {
        double in = inputs[i];
        double out = aa->add_reading(in);
        printf("valid = %d in = %5.1f out = %5.1f\n",
               aa->valid(), in, out);
    }
    printf("\n");
}

int main()
{
    test_dg();
    test_sma();
    test_ema();
    return 0;
}
