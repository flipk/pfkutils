#!/usr/bin/env python3

# write a python function that takes a numpy array of samples
# and runs an fft on it.
# assume the length of the array is the size of the fft,
# and verify this is a power of 2.
# assume the samples are real-valued (not complex).  take an arg
# for the bit width of the samples, for instance if the user passes 14,
# this means the samples should be between 8191 and -8191 (e.g. where
# 8191 is "full scale").
# before taking the fft, scale the values by this range so the input
# data to fft should be between 1.0 and -1.0 (e.g. where 1.0 is
# now "full scale").
# after taking the fft, convert it to a logarithmic scale which represents
# dB below full scale.
# return this fft data.
#
# (note modifications were made after it generated code)

import numpy as np
from scipy import signal
from typing import Union


window: Union[np.ndarray, None] = None
windowsz = 0


def real_fft_dbfs(samples: np.ndarray, bit_width: int) -> np.ndarray:
    """
    Performs a real-valued FFT on a NumPy array of samples, scales the input,
    and returns the result in dBFS (dB below full scale).

    Args:
        samples: A 1-D NumPy array of real-valued samples.
        bit_width: The bit width of the samples.

    Returns:
        A 1-D NumPy array of the FFT result in dBFS.

    Raises:
        ValueError: If the length of the samples array is not a power of 2.
    """

    fft_length = len(samples)
    if (fft_length & (fft_length - 1)) != 0:
        raise ValueError("Length of samples array must be a power of 2.")

    # Apply Flattop Window, normalized to 1.0
    global windowsz
    global window
    if windowsz != fft_length:
        # only rebuild the window when we need to.
        windowsz = fft_length
        # for these dac related signals, the hann window seems to look best.
        # window = signal.windows.flattop(fft_length)
        window = signal.windows.hann(fft_length)
        # window = signal.windows.hamming(fft_length)
        # pycharm complains about the type passed to np.mean, and it
        # is actually correct but i don't know how to make pycharm happy.
        # so, pycharm loses.
        # noinspection PyTypeChecker
        window /= np.mean(window)

    windowed_samples = samples * window
    full_scale = 2**(bit_width - 1) - 1
    scaled_samples = windowed_samples / full_scale  # Scale to [-1.0, 1.0]

    fft_result = np.fft.rfft(scaled_samples)  # Real FFT
    fft_result /= len(fft_result)  # scale by the number of bins

    # Calculate dBFS
    magnitude = np.abs(fft_result)
    # Avoid log10(0) by adding a small epsilon
    dbfs = 20 * np.log10(magnitude + 1e-10)
    dbfs_full_scale = 20 * np.log10(1.0)

    return dbfs - dbfs_full_scale
