#!/usr/bin/env python3

from dofft import real_fft_dbfs
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import sys


def animate_fft(start_freq, end_freq, freq_step, delay_ms,
                sample_rate, duration, bit_width, fft_size):
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
    """

    fig, ax = plt.subplots()
    freq_bins = np.fft.rfftfreq(fft_size, 1 / sample_rate)
    freq_bins /= 10e5  # scale the labels to units of MHz
    line, = ax.plot(freq_bins, np.zeros_like(freq_bins))  # Initialize with zeros
    plt.ylim(-150, 5)

    ax.set_xlabel('Frequency (MHz)')
    ax.set_ylabel('Magnitude (dBFS)')
    # title =
    ax.set_title('FFT (dBFS)')
    ax.grid(True)
    # ax.set_xlim(0, sample_rate / 2)  # Set x-axis limits

    def update(frame):
        frequency = start_freq + frame * freq_step
        if frequency > end_freq:
            return line,

        t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
        orig_signal = np.sin(2 * np.pi * frequency * t[:fft_size])
        full_scale = 2**(bit_width - 1) - 1
        int_signal = (orig_signal * full_scale).astype(np.int64)
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
    print('usage: main.py <testnumber>')


def main() -> int:
    if len(sys.argv) != 2:
        usage()
        return 1
    # these start and end values have strange offsets to avoid
    # exact ratios with the sample rate. when we line up to exact
    # ratios, the noise floor drops out and makes the animation
    # look not as good.
    if sys.argv[1] == "1":
        start_freq = 29111111
        end_freq = 33111111
        freq_step = 20e3
    elif sys.argv[1] == "2":
        start_freq = 20111111
        end_freq = 22111111
        freq_step = 20e3
    elif sys.argv[1] == "3":
        start_freq = 14111111
        end_freq = 16111111
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
                duration, bit_width, fft_size)
    return 0


if __name__ == '__main__':
    exit(main())
