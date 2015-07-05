
#include "pseudo_random.H"
 
pseudo_random_generator :: pseudo_random_generator(uint32_t seed)
{
    c = 362436;
 
    Q[0] = seed;
    Q[1] = seed + PHI;
    Q[2] = seed + PHI + PHI;

    for (int ind = 3; ind < 4096; ind++)
        Q[ind] = Q[ind - 3] ^ Q[ind - 2] ^ PHI ^ ind;

    i = 4095;
}

uint32_t
pseudo_random_generator :: next_value(void)
{
    uint64_t t, a = 18782LL;
    uint32_t x, r = 0xfffffffe;
    i = (i + 1) & 4095;
    t = a * Q[i] + c;
    c = (t >> 32);
    x = t + c;
    if (x < c) {
        x++;
        c++;
    }
    return (Q[i] = r - x);
}
