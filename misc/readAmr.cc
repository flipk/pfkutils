#if 0
set -e -x
g++ readAmr.cc -o readAmr
./readAmr < ringing.amr
exit 0
#endif

#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>

/* number of speech bits for all modes */
int unpacked_size[16] = {
    95, 103, 118, 134, 148, 159, 204, 244,
    35,   0,   0,   0,   0,   0,   0,   0
};

// this table comes straight from the 3gpp specs,
// AMR source code.

int sort_122[244] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
    10,  11,  12,  13,  14,  23,  15,  16,  17,  18,
    19,  20,  21,  22,  24,  25,  26,  27,  28,  38,
    141,  39, 142,  40, 143,  41, 144,  42, 145,  43,
    146,  44, 147,  45, 148,  46, 149,  47,  97, 150,
    200,  48,  98, 151, 201,  49,  99, 152, 202,  86,
    136, 189, 239,  87, 137, 190, 240,  88, 138, 191,
    241,  91, 194,  92, 195,  93, 196,  94, 197,  95,
    198,  29,  30,  31,  32,  33,  34,  35,  50, 100,
    153, 203,  89, 139, 192, 242,  51, 101, 154, 204,
    55, 105, 158, 208,  90, 140, 193, 243,  59, 109,
    162, 212,  63, 113, 166, 216,  67, 117, 170, 220,
    36,  37,  54,  53,  52,  58,  57,  56,  62,  61,
    60,  66,  65,  64,  70,  69,  68, 104, 103, 102,
    108, 107, 106, 112, 111, 110, 116, 115, 114, 120,
    119, 118, 157, 156, 155, 161, 160, 159, 165, 164,
    163, 169, 168, 167, 173, 172, 171, 207, 206, 205,
    211, 210, 209, 215, 214, 213, 219, 218, 217, 223,
    222, 221,  73,  72,  71,  76,  75,  74,  79,  78,
    77,  82,  81,  80,  85,  84,  83, 123, 122, 121,
    126, 125, 124, 129, 128, 127, 132, 131, 130, 135,
    134, 133, 176, 175, 174, 179, 178, 177, 182, 181,
    180, 185, 184, 183, 188, 187, 186, 226, 225, 224,
    229, 228, 227, 232, 231, 230, 235, 234, 233, 238,
    237, 236,  96, 199						
};

int packed_size[16] = {
    12, 13, 15, 17, 19, 20, 26, 31,
    5, 0, 0, 0, 0, 0, 0, 0
};

int
main()
{
    uint8_t header[6];
    int cc;

    cc = read(0, header, 6);
    if (cc != 6)
    {
        printf("bad header\n");
        return 1;
    }

    printf("header: %02x%02x%02x%02x%02x%02x\n",
           header[0], header[1], header[2],
           header[3], header[4], header[5]);

    while (1)
    {
        uint8_t toc;
        if (read(0, &toc, 1) != 1)
            break;
        uint8_t q  = (toc >> 2) & 0x01;
        uint8_t ft = (toc >> 3) & 0x0F;
        uint8_t packed_bits[64];
        if (read (0, packed_bits, packed_size[ft]) != packed_size[ft])
        {
            printf("bad packet\n");
            return 3;
        }

        uint8_t serial_bits[300];
        memset(serial_bits, 0xFF, sizeof(serial_bits));
        uint8_t * ptr = packed_bits;
        for (int ind = 1; ind < (unpacked_size[ft] + 1); ind++)
        {
            if (*ptr & 0x80)
                serial_bits[sort_122[ind-1]] = 1;
            else
                serial_bits[sort_122[ind-1]] = 0;
            if (ind % 8)
                *ptr <<= 1;
            else
                ptr++;
        }

        uint8_t serial_bytes[64];
        memset(serial_bytes,0,sizeof(serial_bytes));
        serial_bytes[0] = ft;
        for (int ind = 0; ind < unpacked_size[ft]; ind++)
        {
            if (serial_bits[ind] == 1)
            {
                int dest_bit = ind + 4;
                serial_bytes[dest_bit >> 3] |= (1 << (dest_bit & 7));
            }
        }

        printf("%d %d ", ft, q);
#if 0
        for (int ind = 0; ind < unpacked_size[ft]; ind++)
            printf("%d", serial_bits[ind]);
#else
        for (int ind = 0; ind < (((unpacked_size[ft]-1)/8)+1); ind++)
            printf("%02x", serial_bytes[ind]);
#endif
        printf("\n");
    }

    return 0;
}
