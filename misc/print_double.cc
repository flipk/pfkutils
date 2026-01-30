#if 0
set -e -x
g++ print_double.cc -o pd
./pd
rm -f pd
exit 0
#endif

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <string>

void print_double(const std::string &s, double x)
{
    uint64_t bits;
    memcpy(&bits, &x, sizeof(bits));

    uint64_t     sign = (bits >> 63) & 1;
    uint64_t exponent = (bits >> 52) & 0x7FF;
    uint64_t mantissa = (bits & 0xFFFFFFFFFFFFF);
    bool    subnormal = (exponent == 0);

    if (subnormal)
      exponent = 1; // IEEE754 rule
    printf("---- arg: '%s'\n", s.c_str());
    printf("bin: %016lx  dec: %.40le\n", bits, x);
    printf("%s%d.", sign ? "-" : "", subnormal ? 0 : 1);
    for (int ind = 51; ind >= 0; ind--)
    {
        uint64_t mask = (1ULL << ind);
        printf("%d", (mantissa & mask) !=0 );
        if ((ind & 3) == 0)
            printf(" ");
    }
    printf("x 2^%ld\n\n", (long)exponent - 1023);
}

#define PRINT_DOUBLE(x)  print_double(#x, x)

int main()
{
    PRINT_DOUBLE(1);
    PRINT_DOUBLE(2);
    PRINT_DOUBLE(4);
    PRINT_DOUBLE(8);
    PRINT_DOUBLE(1.5);
    PRINT_DOUBLE(3);
    PRINT_DOUBLE(6);
    PRINT_DOUBLE(12);
    PRINT_DOUBLE(.3125);
    PRINT_DOUBLE(.625);
    PRINT_DOUBLE(1.25);
    PRINT_DOUBLE(2.5);
    PRINT_DOUBLE(5);
    PRINT_DOUBLE(0.1);
    PRINT_DOUBLE(0.2);
    PRINT_DOUBLE(0.3);
    PRINT_DOUBLE(0);
    PRINT_DOUBLE(1024.03125);
    PRINT_DOUBLE(512.0078125);
    PRINT_DOUBLE(1e-323L);
    PRINT_DOUBLE(1.7976931348e308L);
    return 0;
}
