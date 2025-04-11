#!/usr/bin/env python3

# google gemini:
# write a python function called "generate_cosine_table" which generates and
# returns an array of integers. use numpy arrays. it takes two arguments. the first argument
# is a bit width, which describes the number of bits in a signed integer, for
# instance if the caller passes 14, then the generated integers must be between
# -8192 and 8191. the second arg is the array size to generate.
# the function should verify this size is a power of 2. the value at each position
# in the array is the cosine of an angle. the angle goes from 0 to 2*PI over
# the length of the array.
#
# (note modifications were made after it generated code)

import numpy as np
import math


def generate_cosine_table(data_width: int,
                          output_size: int) -> np.ndarray:
    """
    Generates and returns a NumPy array of integers representing cosine values.

    Args:
        data_width: The number of bits in the signed integer representation.
                   The range of generated integers will be from -(2**(bit_width-1))
                   to (2**(bit_width-1)) - 1.
        output_size: The number of elements to generate in the array. This
                    must be a power of 2.

    Returns:
        A NumPy array of integers representing the cosine values scaled to the
        specified bit width.

    Raises:
        ValueError: If array_size is not a power of 2.
    """
    if (output_size & (output_size - 1)) != 0:
        raise ValueError("array_size must be a power of 2.")

    max_positive = (2 ** (data_width - 1)) - 1
    angles = np.linspace(0, 2 * math.pi, output_size, endpoint=False)
    cosine_values = np.cos(angles)
    scaled_values = np.round(cosine_values * max_positive).astype(np.int64)

    return scaled_values


if __name__ == '__main__':
    try:
        bit_width = 14
        array_size = 16
        cosine_values_np = generate_cosine_table(bit_width, array_size)
        print(f"NumPy cosine table with bit width {bit_width} and size {array_size}:")
        print(cosine_values_np)
        print(f"Data type: {cosine_values_np.dtype}")

        bit_width = 8
        array_size = 256
        cosine_values_np = generate_cosine_table(bit_width, array_size)
        print(f"\nNumPy cosine table with bit width {bit_width} and size {array_size} (first 10 elements):")
        print(cosine_values_np[:10])
        print(f"Data type: {cosine_values_np.dtype}")

        # Example of invalid array size
        bit_width = 10
        array_size = 100
        # This will raise a ValueError
        generate_cosine_table(bit_width, array_size)

    except ValueError as e:
        print(f"Error: {e}")

    bit_width = 14
    array_size = 16384
    cosine_values_np = generate_cosine_table(bit_width, array_size)
    labels = np.linspace(0, 1.0, array_size, endpoint=False)

    import matplotlib.pyplot as plt

    plt.figure()
    plt.plot(labels, cosine_values_np)
    plt.xlabel('binary angle (normalized to 1.0)')
    plt.ylabel('cosine (14-bit signed)')
    plt.title(f'{bit_width} bit cosine lookup table, 16384 entries')
    plt.grid(True)
    plt.show()
