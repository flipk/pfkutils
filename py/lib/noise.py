#!/usr/bin/env python3

import sys
import numpy as np
# https://numpy.org/doc/stable/reference/index.html#reference


def output_csv(data: np.ndarray, fname: str):
    with open(fname, mode='w') as f:
        for ind in range(0, len(data)):
            s = data[ind]
            f.write(f'{ind}, {s.real}, {s.imag}\n')


def make_noise(num_buckets: int) -> (np.ndarray, np.ndarray):
    # NOTE bucket [0] is the DC component.
    #      bucket [1] is the lowest positive freq
    #      bucket [num/2-1] is the highest positive freq
    #      bucket [num/2] is the nyquist freq
    #      bucket [num/2+1] is the highest negative freq
    #      bucket [num-1] is the lowest negative freq
    scale = 100
    rng = np.random.default_rng()
    buckets = (np.exp(1j*np.pi*2*rng.random(num_buckets,
                                            dtype=np.double))
               * scale)
    buckets[0] = 0  # wipe out the DC freq
    ret = np.fft.ifft(buckets)
    m = np.max(np.real(ret))
    n = np.max(np.imag(ret))
    if m > n:
        ret /= m
    else:
        ret /= n
    return buckets, ret


def main(args: list[str]):
    if len(args) < 2:
        return 1
    num_buckets = int(args[1])
    buckets, noise = make_noise(num_buckets)
    output_csv(buckets, "buckets.csv")
    output_csv(noise, "fft.csv")
    return 0


if __name__ == '__main__':
    exit(main(sys.argv))
