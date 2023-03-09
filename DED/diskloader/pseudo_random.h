/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

#include <stdint.h>

// an implementation of the Multiply-with-carry
// pseudo random number generator. code stolen from
// wikipedia and then converted to a C++ object.
// the repeating period of this generator is 
// approximately 2^131104.
// the most important aspect for our purposes is 
// the reproducability of a pseudo random stream 
// from a known seed, so that we only have to store 
// the 32-bit seed in order to verify that a file's 
// contents are intact.

class pseudo_random_generator {
    // phi = (1 + sqrt(5)) / 2
    // 2^32 / phi = 0x9e3779b9
    static const uint32_t PHI = 0x9e3779b9;
    uint32_t Q[4096];
    uint32_t c;
    uint32_t i;
public:
    pseudo_random_generator(uint32_t seed);
    uint32_t next_value(void);
};
