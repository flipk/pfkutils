/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:4 -*- */

// note the code assumes every poly has a '1' in bit zero because
// every code includes the brand new latest bit in the output at each
// position; also note the code assume every poly has a '1' in the
// oldest position because that's how you measure the contraint-length
// and therefore the number of states.

#define CONV_POLY_0  0x19
#define CONV_POLY_1  0x1b
#define CONV_POLY_2  0x15
#define CONV_POLY_3  0x1f
#define CONV_POLY_4  0x6d
#define CONV_POLY_5  0x53
#define CONV_POLY_6  0x5f
#define CONV_POLY_7  0x4f
#define CONV_POLY_a1 0x11d
#define CONV_POLY_a2 0x1af
#define CONV_POLY_a3 0x1ed
#define CONV_POLY_a4 0x19b
#define CONV_POLY_a5 0x127

