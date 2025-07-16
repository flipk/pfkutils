#if 0
set -e -x
g++ -Wall -Werror test_parse_complex.cc -o test_parse_complex
./test_parse_complex
exit 0
#endif

#include <complex>
#include <string>
#include <iostream>
#include "parse_complex.h"

void test_case(ComplexConverter &cvt, const std::string &s)
{
    std::complex<double>  result;

    if (!cvt.convert(s, result))
    {
        std::cerr << "Error parsing '" << s << "'\n";
        return;
    }

    std::cout << "parsing: '" << s << "': " << result << "\n";
}

int main(int argc, char ** argv)
{
    ComplexConverter   cvt;

    test_case(cvt, "-4.9e-6+3.1e2j");
    test_case(cvt, "1+2e6j");
    test_case(cvt, "-4");
    test_case(cvt, "-4e3");
    test_case(cvt, "-4j");
    test_case(cvt, "-4e3j");
    test_case(cvt, "4");
    test_case(cvt, "4e3");
    test_case(cvt, "4j");
    test_case(cvt, "4e3j");

    return 0;
}
