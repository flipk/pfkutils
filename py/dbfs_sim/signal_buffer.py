
import numpy as np


class SignalBuffer:
    """
    a buffer of real-valued samples. includes methods to add
    a sin wave of a specified frequency, and calculating the fft.
    """
    sample_rate: int
    num_samples: int
    max_scale: float
    samples: np.array

    def __init__(self, sample_rate: int, num_samples: int, max_scale: float):
        """
        create an empty signal buffer.
        @param sample_rate: int: sample rate for the buffer.
        @param num_samples: int: how many samples in this buffer?
        @param max_scale: float: used during normalization
        """
        self.sample_rate = sample_rate
        self.num_samples = num_samples
        self.max_scale = max_scale
        self.samples = np.zeros(num_samples, np.double)

    def add_sin(self, freq: float, magnitude: float):
        """
        add a sin wave to the signal buffer. the freq is absolute (the
        configured sample rate will be used to scale the sin(w) angle
        increment)
        @param freq: frequency of the sin wave to add
        @param magnitude: magnitude of the sin wave (peak)
        @return: None
        """
        # print(f'adding freq {freq}')
        one_cycle = self.sample_rate / freq
        # print(f'one cycle: {one_cycle} samples')
        radian_step = (np.pi * 2) / one_cycle
        # print(f'radians step: {radian_step}')
        self.samples += np.cos(np.arange(0, self.num_samples) * radian_step) * magnitude

    def normalize(self):
        """
        find the maximum magnitude in the signal buffer, and
        scale all entries so the largest is SignalBuffer(max_scale)
        @return: None
        """
        maximum = np.max(self.samples)
        self.samples /= maximum
        self.samples *= self.max_scale

    def write_fft_csv(self, fname: str,
                      buckets: int, freq_scale: float = 1):
        """
        calculate the FFT of the signal buffer data and write the result
        to an output CSV file. two columns are created:
        {frequency, magnitude}
        @type fname: str: specify the name of the CSV file to create
        @type buckets: int: what size FFT to use (should be even #)
        @type freq_scale: float: scale the bucket numbers, e.g. if you
            specify "1e6", the output will be in units of MHz
        @return None
        """
        fftdata = np.fft.fft(self.samples[0:buckets])
        # at this point,
        #   data[0] = is the DC freq
        #   data[1] = is the lowest positive freq
        #   data[buckets/2-1] is highest positive freq below nyquist
        #   data[buckets/2] is the highest (nyquist) freq
        #   data[buckets/2+1] is highest negative freq below nyquist
        #   data[buckets-1] is the lowest negative freq
        #
        # don't do this, it's not useful for real-valued data:
        #   fftdata = np.fft.fftshift(fftdata)
        # if you did, the data would be:
        #   data[0] = nyquist freq
        #   data[1] = highest negative freq
        #   data[buckets/2-1] = lowest negative freq
        #   data[buckets/2] = DC freq
        #   data[buckets/2+1] = lowest positive freq
        #   data[buckets-1] = highest positive freq
        mags = np.abs(fftdata) / self.max_scale / (buckets/2)
        freq = 0
        freq_step = self.sample_rate / buckets / freq_scale
        with open(fname, mode='wt') as f:
            for ind in range(0, int(buckets/2)):
                f.write(f'{freq} {mags[ind]}\n')
                freq += freq_step

    def write_csv(self, fname: str):
        """
        Write the signal buffer to a CSV file. two columns are
        created: {sample count, sample value}. sample count starts
        at zero.
        @param fname: str: filename of the CSV file to create
        @return: None
        """
        count = 0
        with open(fname, mode='wt') as f:
            for ind in range(0, self.num_samples):
                f.write(f'{count} {self.samples[ind]}\n')
                count += 1
