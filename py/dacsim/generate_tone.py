#!/usr/bin/env python3

# google gemini:
# write a python function called generate_tone which creates and returns
# a numpy array of integers. the first arg is the sample rate in units of
# samples per second. the second arg is the frequency of tone to create in hertz.
# the third arg is the size of the array to create. the fourth and fifth args are
# the bit width and array size for the function you just wrote above.
#    (note this is referring to generate_cosine_table)
#
# this method is intended to simulate an FPGA module which generates a tone
# to a digital to analog converter and should use the following technique.
# it should call the function above to make a cosine lookup table.
# since the lookup table should be a power of 2 in size, there should be a
# specific number of bits required to index that table. call that table_lookup_bits.
#
# there is a 32-bit integer which serves as a phase accumlator. this value starts
# at 0. for each entry in the output array, the top table_lookup_bits of that
# accumulator value will be used to index the cosine lookup table and populate
# the output. the accumulator should then be incremented by an integer
# value corresponding to the sample rate and tone frequency.
#
# (note modifications were made after it generated code)

import numpy as np
import math
from typing import Union
import random

from generate_cosine_table import generate_cosine_table

_cosine_table: Union[np.ndarray, None] = None
_cosine_table_bit_width = 0
_cosine_table_size = 0
_cosine_table_lookup_bits = 0


def _gentable(cosine_table_bit_width: int, cosine_table_size: int):
    """only regenerate the cosine table if it doesn't exist or if the
    parameters have changed"""
    global _cosine_table
    global _cosine_table_bit_width
    global _cosine_table_size
    global _cosine_table_lookup_bits
    if cosine_table_bit_width != _cosine_table_bit_width or \
       cosine_table_size != _cosine_table_size:
        _cosine_table = generate_cosine_table(cosine_table_bit_width, cosine_table_size)
        _cosine_table_lookup_bits = int(math.log2(cosine_table_size))
        _cosine_table_bit_width = cosine_table_bit_width
        _cosine_table_size = cosine_table_size


def generate_tone(output_sample_rate: float, frequency: float, output_array_size: int,
                  cosine_table_bit_width: int, cosine_table_size: int,
                  dither) -> np.ndarray:
    """
    Generates a NumPy array of integers representing a sinusoidal tone using a
    cosine lookup table and a phase accumulator.

    Args:
        output_sample_rate: The sample rate in samples per second (Hz).
        frequency: The frequency of the tone to generate in Hertz.
        output_array_size: The number of samples to generate in the output array.
        cosine_table_bit_width: The bit width for the cosine lookup table integers.
        cosine_table_size: The size of the cosine lookup table (must be a power of 2).
        dither: apply random dithering to samples to reduce harmonics

    Returns:
        A NumPy array of integers representing the generated tone.
    """
    if (cosine_table_size & (cosine_table_size - 1)) != 0:
        raise ValueError("cosine_table_size must be a power of 2.")

    _gentable(cosine_table_bit_width, cosine_table_size)
    phase_accumulator = 0
    phase_increment = int((frequency * (2**32)) / output_sample_rate)
    output_array = np.zeros(output_array_size, dtype=np.int64)

    # Mask to extract the top 'table_lookup_bits'
    mask = (2**32 - 1) >> (32 - _cosine_table_lookup_bits)

    # if dithering, this is the amount to dither by.
    dithermult = 1 << (32 - _cosine_table_lookup_bits)
    offset = 0
    for i in range(output_array_size):
        if dither:
            offset = int((random.random() - 0.5) * dithermult)
        table_index = ((phase_accumulator + offset) >> (32 - _cosine_table_lookup_bits)) & mask
        output_array[i] = _cosine_table[table_index]
        phase_accumulator = (phase_accumulator + phase_increment)

    return output_array


if __name__ == '__main__':
    sample_rate = 125e6
    tone_frequency = 24e6
    output_size = 256
    cosine_bit_width = 12
    cosine_table_length = 2048  # Must be a power of 2

    try:
        tone = generate_tone(sample_rate, tone_frequency, output_size,
                             cosine_bit_width, cosine_table_length,
                             dither=False)
        print(f"Generated tone array of size: {tone.shape}")
        print(f"First 100 elements of the tone:\n{tone[:100]}")
        print(f"Data type of the tone array: {tone.dtype}")

        import matplotlib.pyplot as plt

        plt.figure()
        plt.plot(tone)
        plt.xlabel('time')
        plt.ylabel('sample value')
        plt.title(f'{tone_frequency} Hz at {sample_rate} S/s')
        plt.grid(True)
        plt.show()

    except ValueError as e:
        print(f"Error: {e}")
