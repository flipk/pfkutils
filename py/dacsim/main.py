#!/usr/bin/env python3

from dofft import real_fft_dbfs
from generate_tone import generate_tone
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import sys


def animate_fft(start_freq, end_freq, freq_step, delay_ms,
                sample_rate, bit_width, fft_size,
                cosine_table_size, target_freq, dither):
    """
    Animates the FFT of a sine wave over a range of frequencies.

    Args:
        start_freq: Starting frequency (Hz).
        end_freq: Ending frequency (Hz).
        freq_step: Frequency step size (Hz).
        delay_ms: Delay between frames (milliseconds).
        sample_rate: Sample rate (Hz).
        bit_width: Bit width of the samples.
        fft_size: the number of buckets to use in the fft
        target_freq: put the target freq in the plot title
        cosine_table_size: number of entries in the table
        dither: apply random dithering to samples to reduce harmonics
    """

    fig, ax = plt.subplots()
    freq_bins = np.fft.rfftfreq(fft_size, 1 / sample_rate)
    freq_bins /= 10e5  # scale the labels to units of MHz
    line, = ax.plot(freq_bins, np.zeros_like(freq_bins))  # Initialize with zeros
    plt.ylim(-150, 5)

    ax.set_xlabel('Frequency (MHz)')
    ax.set_ylabel('Magnitude (dBFS)')
    ax.set_title(f'FFT (dBFS), target freq {target_freq:.3f} MHz')
    ax.grid(True)
    # ax.set_xlim(0, sample_rate / 2)  # Set x-axis limits

    def update(frame):
        frequency = start_freq + frame * freq_step
        if frequency > end_freq:
            return line,

        orig_signal = generate_tone(sample_rate, frequency, 
                                    output_array_size=fft_size,
                                    cosine_table_bit_width=bit_width,
                                    cosine_table_size=cosine_table_size,
                                    dither=dither)

        fft_dbfs = real_fft_dbfs(orig_signal, bit_width)
        line.set_ydata(fft_dbfs)
        ax.set_title(f'FFT (dBFS) ({frequency/1e6:.3f} MHz, target {target_freq:.3f} MHz)')
        return line,

    num_frames = int((end_freq - start_freq) / freq_step) + 1
    # you don't need the return value, but if you don't collect
    # the return value, it doesn't do anything, because the return
    # value is the object which does the work; if you don't keep it,
    # it gets garbage-collected immediately.
    # also, updating the title doesn't seem to work when blit=True,
    # even if the update function returns "ax" in the list of artists.
    # the only workaround to get the title to animate properly seems
    # to be to just disable blitting.
    # noinspection PyUnusedLocal
    ani = animation.FuncAnimation(fig, update, frames=num_frames,
                                  interval=delay_ms, blit=False)
    plt.show()
    print('')


def usage():
    print('usage: main.py <dither> <testnumber>')


def main() -> int:
    if len(sys.argv) != 3:
        usage()
        return 1
    if sys.argv[1] == "y":
        dither = True
    elif sys.argv[1] == "n":
        dither = False
    else:
        usage()
        return 1
    # these start and end values have strange offsets to avoid
    # exact ratios with the sample rate. when we line up to exact
    # ratios, the noise floor drops out and makes the animation
    # look not as good.
    if sys.argv[2] == "1":
        target_freq = 125.0 / 4
        start_freq = 31240003
        end_freq = 31260003
        freq_step = 0.1e3
    elif sys.argv[2] == "2":
        target_freq = 125.0 / 6
        start_freq = 20823333
        end_freq = 20843333
        freq_step = 0.1e3
    elif sys.argv[2] == "3":
        target_freq = 125.0 / 8
        start_freq = 15615003
        end_freq = 15635003
        freq_step = 0.1e3
    else:
        usage()
        return 1

    delay_ms = 50
    sample_rate = 125e6
    bit_width = 14
    fft_size = 16384
    cosine_table_size = 2048

    animate_fft(start_freq, end_freq, freq_step, delay_ms, sample_rate,
                bit_width, fft_size, cosine_table_size,
                target_freq, dither)
    return 0


if __name__ == '__main__':
    exit(main())
