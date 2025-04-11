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
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import sys


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

def animate_fft(start_freq, end_freq, freq_step, delay_ms,
                sample_rate, duration, bit_width, fft_size,
                quantized, target_freq):
    """
    Animates the FFT of a sine wave over a range of frequencies.

    Args:
        start_freq: Starting frequency (Hz).
        end_freq: Ending frequency (Hz).
        freq_step: Frequency step size (Hz).
        delay_ms: Delay between frames (milliseconds).
        sample_rate: Sample rate (Hz).
        duration: Duration of the signal (seconds).
        bit_width: Bit width of the samples.
        fft_size: the number of buckets to use in the fft
        quantized: should it be quantized to integers
    """

    fig, ax = plt.subplots()
    freq_bins = np.fft.rfftfreq(fft_size, 1 / sample_rate)
    freq_bins /= 10e5  # scale the labels to units of MHz
    line, = ax.plot(freq_bins, np.zeros_like(freq_bins))  # Initialize with zeros
    plt.ylim(-150, 5)

    ax.set_xlabel('Frequency (MHz)')
    ax.set_ylabel('Magnitude (dBFS)')
    # title =
    ax.set_title(f'FFT (dBFS), target freq {target_freq:.3f} MHz')
    ax.grid(True)
    # ax.set_xlim(0, sample_rate / 2)  # Set x-axis limits

    def update(frame):
        frequency = start_freq + frame * freq_step
        if frequency > end_freq:
            return line,

        t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
        orig_signal = np.sin(2 * np.pi * frequency * t[:fft_size])
        full_scale = 2**(bit_width - 1) - 1
        if quantized:
            int_signal = (orig_signal * full_scale).astype(np.int64)
        else:
            int_signal = orig_signal * full_scale
        fft_dbfs = real_fft_dbfs(int_signal, bit_width)
        line.set_ydata(fft_dbfs)
        print(f'\r  freq: {frequency/1e6:.2f} MHz  <hit q to stop>', end='')
        # this does NOT work for some reason
        # title.set_text(f'FFT (dBFS) - Frequency: {frequency:.2f} Hz')
        return line,  # title

    num_frames = int((end_freq - start_freq) / freq_step) + 1
    # you don't need the return value, but if you don't collect
    # the return value, it doesn't do anything, because the return
    # value is the object which does the work; if you don't keep it,
    # it gets garbage-collected immediately.
    ani = animation.FuncAnimation(fig, update, frames=num_frames, interval=delay_ms, blit=True)
    plt.show()
    print('')


def usage():
    print('usage: dofft.py <quantized> <testnumber>')
    print('    quantized: "y" means 14 bit signed integers, "n" means float')


def main() -> int:
    if len(sys.argv) != 3:
        usage()
        return 1
    if sys.argv[1] == "y":
        quantized = True
    elif sys.argv[1] == "n":
        quantized = False
    else:
        usage()
        return 1
    # these start and end values have strange offsets to avoid
    # exact ratios with the sample rate. when we line up to exact
    # ratios, the noise floor drops out and makes the animation
    # look not as good.
    if sys.argv[2] == "1":
        target_freq = 125.0 / 4
        start_freq = 29000003
        end_freq = 33000003
        freq_step = 20e3
    elif sys.argv[2] == "2":
        target_freq = 125.0 / 6
        start_freq = 20000003
        end_freq = 22000003
        freq_step = 20e3
    elif sys.argv[2] == "3":
        target_freq = 125.0 / 8
        start_freq = 14000003
        end_freq = 16000003
        freq_step = 20e3
    else:
        usage()
        return 1

    delay_ms = 50
    sample_rate = 125e6
    duration = 0.001
    bit_width = 14
    fft_size = 16384

    animate_fft(start_freq, end_freq, freq_step, delay_ms, sample_rate,
                duration, bit_width, fft_size, quantized, target_freq)
    return 0


if __name__ == '__main__':
    exit(main())
