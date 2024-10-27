#!/usr/bin/env python3

import sys
from signal_buffer import SignalBuffer

sample_rate = 125000000
max_mag = 1000
buckets = 8192
num_samples = 10000

# these frequencies are rounded to the exact center freqs
# of the FFT buckets of interest.
freq1 = 2.01416015625e6
# freq1 = 2e6
freq2 = 12.5732421875e6
# freq2 = 12.5e6
freq3 = 21.026611328125e6
# freq3 = 21e6


# noinspection PyUnusedLocal
def main(args: list[str]) -> int:
    sb = SignalBuffer(sample_rate, num_samples, max_mag)
    sb.add_sin(freq1, 1.0)
    sb.add_sin(freq2, 1.0)
    sb.add_sin(freq3, 1.0)
    sb.normalize()
    sb.write_csv('combined.csv')
    sb.write_fft_csv('fft.csv',
                     buckets, 1e6)
    return 0


if __name__ == '__main__':
    exit(main(sys.argv))
