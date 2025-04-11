#!/usr/bin/env python3

# google gemini:
# write a python function which takes a numpy array of samples as an argument,
# upsamples it by 5x, constructs a low pass FIR filter of 0.18 normalized
# frequency (but make the 0.18 value a top level global variable so i can
# easily tweak it later) and returns the upsampled result.
#
# (note modifications were made after it generated code)

import numpy as np
from scipy import signal

UPSAMPLING_FACTOR = 5
LOW_PASS_CUTOFF_NORMALIZED = 0.18
FIR_FILTER_TAPS = 101
low_pass_filter = signal.firwin(FIR_FILTER_TAPS, LOW_PASS_CUTOFF_NORMALIZED, window='hamming')


def upsample_and_filter(samples: np.ndarray) -> np.ndarray:
    """
    Upsamples a NumPy array of samples by a factor of 5 and applies a
    low-pass FIR filter to prevent aliasing.

    Args:
        samples: A 1-D NumPy array of audio samples.

    Returns:
        A 1-D NumPy array of the upsampled and filtered samples.
    """
    # 1. Upsample by inserting zeros
    upsampled_samples = np.zeros(len(samples) * UPSAMPLING_FACTOR)
    upsampled_samples[::UPSAMPLING_FACTOR] = samples

    # 2. Apply the lowpass filter
    filtered_samples = signal.lfilter(low_pass_filter, 1.0, upsampled_samples)

    return filtered_samples


if __name__ == '__main__':
    # Example usage:
    original_sample_rate = 1000  # Hz
    duration = 1  # seconds
    frequency = 170  # Hz
    t = np.linspace(0, duration, int(original_sample_rate * duration), endpoint=False)
    original_signal = np.sin(2 * np.pi * frequency * t)

    upsampled_filtered_signal = upsample_and_filter(original_signal)

    new_sample_rate = original_sample_rate * UPSAMPLING_FACTOR
    t_upsampled = np.linspace(0, duration, len(upsampled_filtered_signal), endpoint=False)

    import matplotlib.pyplot as plt

    plt.figure(figsize=(12, 6))

    plt.subplot(2, 1, 1)
    plt.plot(t[:100], original_signal[:100])
    plt.title(f'{original_sample_rate} Hz tone @ {original_sample_rate} S/s)')
    plt.xlabel('Time (s)')
    plt.ylabel('Amplitude')

    plt.subplot(2, 1, 2)
    plt.plot(t_upsampled[:500], upsampled_filtered_signal[:500])
    plt.title(f'Upsampled to {new_sample_rate} S/s)')
    plt.xlabel('Time (s)')
    plt.ylabel('Amplitude')

    plt.tight_layout()
    plt.show()
