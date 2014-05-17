#if 0
set -e -x
g++ CircularReaderTest.cc -o crt && ./crt
exit 0
#endif
//this file is not compiled as part of the
//library. i just keep it here so i can retest
//crap if i need to.
#include "CircularReader.h"
#include <iostream>
#include <inttypes.h>

using namespace std;

int
main()
{
    unsigned char buf[4] = { 0xa0, 0xb0, 0xb1, 0xb2 };

    CircularReader x;

    x.assign((char*)buf, 4);

    int a = ((uint8_t)x[0]) << 8;

    cout << "a = " << a << endl;

    return 0;
}
