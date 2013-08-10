
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
