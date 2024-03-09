
import socket
import os
import matplotlib.backend_bases
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import gridspec
import struct
import select
import threading
import queue
from SpeedTiming import SpeedTiming


class PlotStatus:
    q: queue.Queue
    ready_for_next_packet: bool
    run: bool
    fig1: plt.Figure | None
    gs: gridspec.GridSpec | None
    plot1: plt.Axes | None
    plot2: plt.Axes | None
    plot3: plt.Axes | None
    plot1line: plt.Line2D | None
    plot2line: plt.Line2D | None
    plot3line: plt.Line2D | None
    plot1txt: plt.Text | None
    plot2txt: plt.Text | None
    plot3txt: plt.Text | None

    def __init__(self):
        self.q = queue.Queue()
        self.ready_for_next_packet = False
        self.run = True
        self.fig1 = None
        self.plot1 = None
        self.plot2 = None
        self.plot3 = None
        self.gs = None
        self.plot1line = None
        self.plot2line = None
        self.plot3line = None
        self.plot1txt = None
        self.plot2txt = None
        self.plot3txt = None

    def on_key(self, event: matplotlib.backend_bases.KeyEvent):
        if event.key == 'q':
            self.run = False

    def on_click(self, event: matplotlib.backend_bases.MouseEvent):
        # get the x and y pixel coords
        # x, y = event.x, event.y
        if event.inaxes == self.plot1:
            # ax = event.inaxes  # the axes instance
            print('frequency %f amplitude %f' % (event.xdata, event.ydata))


def build_flat_top(n_raw):
    flat_top = [None]*n_raw
    k = 0
    while k < n_raw:
        flat_top[k] = 1 - 1.930*np.cos(2*np.pi*k/(n_raw-1)) + \
                          1.290*np.cos(4*np.pi*k/(n_raw-1)) - \
                          0.388*np.cos(6*np.pi*k/(n_raw-1)) + \
                          0.032*np.cos(8*np.pi*k/(n_raw-1))
        k += 1
    return np.array(flat_top)


def receiver(ps: PlotStatus):
    sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    sock.bind(('', 25000))
    while ps.run:
        buf = sock.recv(1600)
        # discard all incoming packets until the plotter
        # says it's ready for the next packet.
        # TODO validate sequence numbers, and get more than one packet
        #      in a row to increase our # samples.
        if ps.ready_for_next_packet:
            ps.ready_for_next_packet = False
            ps.q.put(buf)


def fd0reader(ps: PlotStatus):
    print('press ENTER here, or type q in the window, to stop')
    while ps.run:
        x, y, z = select.select([0], [], [], 0.1)
        if len(x) > 0:
            ps.run = False


def main(do_timings: bool):
    ps = PlotStatus()
    rcvr = threading.Thread(target=receiver, args=[ps], daemon=True)
    rcvr.start()
    fd0rdr = threading.Thread(target=fd0reader, args=[ps], daemon=True)
    fd0rdr.start()

    n_raw = 0                 # Number of samples to collect
    adc_max = (2**16)/2-1       # max (signed) ADC count

    fs = 5e6           # sample rate in Hz
    fc_m_hz = 7         # ddc center frequency in MHz
    fsamp_msps = fs / 1e6
    fbw_m_hz = fsamp_msps * 2   # double sample rate

    window = None     # will be rebuilt later.
    zero_buff = np.zeros(1024)
    plt.figure(figsize=(10, 10), dpi=80)
    plt.connect('button_press_event', ps.on_click)
    plt.connect('key_press_event', ps.on_key)
    plt.ion()
    plt.show()

    ps.fig1 = plt.figure(1)
    ps.gs = gridspec.GridSpec(ncols=1, nrows=3, width_ratios=[1], height_ratios=[3, 1, 1])
    ps.plot1 = ps.fig1.add_subplot(ps.gs[0])
    ps.plot2 = ps.fig1.add_subplot(ps.gs[1])
    ps.plot3 = ps.fig1.add_subplot(ps.gs[2])

    fxmin = -fs / 2
    fxmax = fs / 2
    fymin = -120
    fymax = 0

    ps.plot1.axis([fxmin, fxmax, fymin, fymax])
    ps.plot1.set_xlabel(f'Frequency [MHz], relative to Fc = {fc_m_hz} MHz')
    ps.plot1.set_ylabel('dBFS')
    ps.plot1line, = ps.plot1.plot([], [])
    ps.plot1txt = ps.plot1.text(-fs/4, -14, '')

    ps.plot2.set_xlabel("I Sample Number [n]")
    ps.plot2.set_ylabel("I samples")
    ps.plot2line, = ps.plot2.plot([])
    ps.plot2txt = ps.plot2.text(5, 0, '')

    ps.plot3.set_xlabel("Q Sample Number [n]")
    ps.plot3.set_ylabel("Q samples")
    ps.plot3line, = ps.plot3.plot([])
    ps.plot3txt = ps.plot3.text(5, 0, '')

    ps.ready_for_next_packet = True
    while ps.run:
        timing = SpeedTiming('spectrum', enabled=do_timings)
        try:
            buf = ps.q.get(timeout=0.5)
        except queue.Empty:
            ps.ready_for_next_packet = True
            continue
        ps.ready_for_next_packet = True
        timing.mark('q get')
        # decode the number of words in the packet from
        # the vita49 header word.
        v = struct.unpack_from(">h", buf, 2)
        # the header length is hardcoded to include stream_id
        # and is thus 5 words long.
        # one "vita49 word" (32 bits) contains one IQ sample
        n_raw_new = v[0] - 5
        if n_raw != n_raw_new:
            timing.mark('build flat top')
            window = build_flat_top(n_raw_new)
            n_raw = n_raw_new

        timing.mark('examine header')
        i_samples = [None]*n_raw
        q_samples = [None]*n_raw

        pos = 20
        index = 0
        timing.mark('allocate sample arrays')
        while index < n_raw:
            iq_pair = struct.unpack_from('>hh', buf, pos)
            # NOTE we are swapping I and Q!
            i_samples[index] = iq_pair[1]
            q_samples[index] = iq_pair[0]
            index += 1
            pos += 4

        timing.mark('unpack')
        iq_raw = [np.array(i_samples), np.array(q_samples)]
        iq_windowed = [iq_raw[0]*window, iq_raw[1]*window]
        iq_buffered = [np.append(iq_windowed[0], zero_buff),
                       np.append(iq_windowed[1], zero_buff)]
        n_buff = len(iq_buffered[0])
        iq = (iq_buffered[0]+1j*iq_buffered[1]) / adc_max
        timing.mark('window')
        fs_voltage = np.abs(np.fft.fftshift(np.fft.fft(iq))) / n_raw
        timing.mark('fft')
        d_b = 20*np.log10(fs_voltage)
        d_b_fs = d_b.max()
        freqs_d = np.array([None]*n_buff)
        for i in range(0, len(freqs_d)):
            freqs_d[i] = -n_buff/2 + i
        freqs = freqs_d*fs/n_buff

        i_max = iq_raw[0].max(initial=0)
        q_max = iq_raw[1].max(initial=0)
        i_mean = iq_raw[0].mean()
        q_mean = iq_raw[1].mean()
        i_max_ceil = i_max + (i_max/3)
        q_max_ceil = q_max + (q_max/3)
        timing.mark('math')

        ps.plot1txt.set_text(f'Fsamp: {fsamp_msps} MSPS\n'
                             f'BW: {fbw_m_hz} MHz (x2)\n'
                             f'peak dBFS: {d_b_fs:5.1f}')
        ps.plot1line.set_data(freqs, d_b)

        ps.plot2.axis([0, n_raw, -i_max_ceil, i_max_ceil])
        ps.plot2txt.set(text=f'Max {i_max:5d}   Mean {i_mean:5.1f}', y=i_max_ceil)
        ps.plot2line.set_data(range(0, n_raw), iq_raw[0])

        ps.plot3.axis([0, n_raw, -q_max_ceil, q_max_ceil])
        ps.plot3txt.set(text=f'Max {q_max:5d}   Mean {q_mean:5.1f}', y=q_max_ceil)
        ps.plot3line.set_data(range(0, n_raw), iq_raw[1])
        timing.mark('plot')

        plt.draw()
        timing.mark('draw')

        plt.pause(0.1)
        timing.mark('pause')
        timing.print()


if __name__ == "__main__":
    try:
        main('TIMING' in os.environ)
    except KeyboardInterrupt:
        pass
    exit(0)
