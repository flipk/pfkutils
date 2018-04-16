
#include "downsampler.h"

#include <math.h>
#include <vector>
#include <stdio.h>

FILTERCLASS(double,01,40);
FILTERCLASS_COEFF(double,01,40) = {
-7.77666e-05,-0.000519986,-0.00113996,-0.00203459,-0.00320905,-0.00453283,-0.00571957,-0.00633861,-0.00586062,-0.00373305,0.00052521,0.00722516,0.0164224,0.0278617,0.0409643,0.0548633,0.0684859,0.0806727,0.0903177,0.0965074,0.0986405,0.0965074,0.0903177,0.0806727,0.0684859,0.0548633,0.0409643,0.0278617,0.0164224,0.00722516,0.00052521,-0.00373305,-0.00586062,-0.00633861,-0.00571957,-0.00453283,-0.00320905,-0.00203459,-0.00113996,-0.000519986,-7.77666e-05
};

FILTERCLASS(double,05,40);
FILTERCLASS_COEFF(double,05,40) = {
-7.82233e-05,-0.00143531,0.000100237,0.00243755,-0.000164124,-0.00455924,0.000263631,0.00811458,-0.000389016,-0.0135557,0.000528007,0.0216654,-0.000666998,-0.0340893,0.000792384,0.0551497,-0.00089189,-0.100908,0.000955777,0.316904,0.499651,0.316904,0.000955777,-0.100908,-0.00089189,0.0551497,0.000792384,-0.0340893,-0.000666998,0.0216654,0.000528007,-0.0135557,-0.000389016,0.00811458,0.000263631,-0.00455924,-0.000164124,0.00243755,0.000100237,-0.00143531,-7.82233e-05
};

FILTERCLASS(double,09,40);
FILTERCLASS_COEFF(double,09,40) = {
-7.80145e-05,-0.000363155,0.000982829,-0.00189374,0.00312113,-0.00455167,0.00590601,-0.00674504,0.00651352,-0.00461788,0.000527265,0.00611841,-0.0154127,0.0271177,-0.0406454,0.0550912,-0.0693211,0.0820986,-0.0922378,0.0987558,0.899268,0.0987558,-0.0922378,0.0820986,-0.0693211,0.0550912,-0.0406454,0.0271177,-0.0154127,0.00611841,0.000527265,-0.00461788,0.00651352,-0.00674504,0.00590601,-0.00455167,0.00312113,-0.00189374,0.000982829,-0.000363155,-7.80145e-05
};

int
main()
{
    cplx_downsampler<fir_filter_double_lp01_40>  ds(1000000, 100000);
//    cplx_downsampler<fir_filter_double_lp05_40>  ds(1000000, 500000);
//    cplx_downsampler<fir_filter_double_lp09_40>  ds(1000000, 900000);
    std::vector<cplx_double>  samples_in;

    double angle = 0.0;
    double incr = M_PI * 2 / 80;

    for (int ind = 0; ind < 1000; ind++)
    {
        samples_in.push_back(cplx_double(cos(angle),sin(angle)));
        angle += incr;
    }

    FILE * f = fopen("input.txt", "w");
    for (int ind = 0; ind < samples_in.size(); ind++)
        fprintf(f, "%d %f %f\n", ind,
                samples_in[ind].real(), samples_in[ind].imag());
    fclose(f);

    std::vector<cplx_double> samples_out;

    for (int ind = 0; ind < samples_in.size(); ind++)
    {
        cplx_double out;
        if (ds.add_sample(samples_in[ind] * 2.0, out))
            samples_out.push_back(out);
        if (ds.add_sample(0, out))
            samples_out.push_back(out);
    }

    f = fopen("output.txt", "w");
    for (int ind = 0; ind < samples_out.size(); ind++)
        fprintf(f, "%d %f %f\n", ind,
                samples_out[ind].real(), samples_out[ind].imag());
    fclose(f);

    return 0;
}
