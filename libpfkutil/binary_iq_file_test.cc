#if 0
set -e -x
g++ binary_iq_file_test.cc -o binary_iq_file_test
./binary_iq_file_test
exit 0
;
#endif

#include "binary_iq_file.h"

int
main()
{
    binary_iq_file_config  cfg;

    cfg.bits = binary_iq_file_config::BITS_12;
    cfg.first = binary_iq_file_config::Q_FIRST;
    cfg.endian = binary_iq_file_config::LITTLE;
    cfg.ts = binary_iq_file_config::NOTS;
    cfg.rep = binary_iq_file_config::COMPL2;
    cfg.ts_interval = 0;
    cfg.binary_offset = 0;

    binary_iq_file_writer wrtr("testiq.bin", cfg);

    double angle = 0.0;
    double incr = M_PI * 2 / 80;
    for (int ind = 0; ind < 1000000; ind++)
    {
        cplx_double sample(cos(angle), sin(angle));
        angle += incr;
        wrtr.put_sample(sample);
    }

    return 0;
}
