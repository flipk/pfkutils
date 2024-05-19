#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
from matplotlib import gridspec
from matplotlib.axes import Axes
from matplotlib.figure import Figure
from matplotlib.gridspec import GridSpec
from matplotlib.lines import Line2D


class MyPlot:
    fig0: Figure
    gs: GridSpec
    fig1: Figure
    plot1: Axes
    plot2: Axes
    plot1line: Line2D
    plot2line: Line2D

    def __init__(self):
        # do not force-raise window on EVERY SINGLE REPLOT.
        # why would ANYONE want that????
        plt.rcParams["figure.raise_window"] = False
        self.fig0 = plt.figure(figsize=(10, 10), dpi=80)
        plt.ion()
        plt.show()  # needed?
        self.fig1 = plt.figure(1)
        self.gs = gridspec.GridSpec(ncols=1, nrows=2,
                                    width_ratios=[1], height_ratios=[0.5, 0.5])
        self.plot1 = self.fig1.add_subplot(self.gs[0])
        self.plot1.set_xlabel('sample')
        self.plot1.set_ylabel('amplitude')

        self.plot2 = self.fig1.add_subplot(self.gs[1])
        self.plot2.set_xlabel('frequency')
        self.plot2.set_ylabel('magnitude')

        self.plot1line1, = self.plot1.plot([], [], 'k')
        self.plot1line2, = self.plot1.plot([], [], 'r')
        self.plot2line, = self.plot2.plot([], [], 'k')

    def plot(self, xdata1, xdata2, ydata1, ydata2):
        xmin1 = np.min(xdata1)
        xmax1 = np.max(xdata1)
        ymin1 = np.min(ydata1)
        ymax1 = np.max(ydata1)
        ymin2 = np.min(ydata2)
        ymax2 = np.max(ydata2)

        xrange1 = xmax1 - xmin1
        yrange1 = ymax1 - ymin1
        yrange2 = ymax2 - ymin2

        xmin1 -= xrange1 / 25
        xmax1 += xrange1 / 25
        ymax1 += yrange1 / 25
        ymin1 -= yrange1 / 25
        ymax2 += yrange2 / 25
        ymin2 -= yrange2 / 25

        xdata2_len = xdata2.shape[0]
        print(f'shape of xdata: {xdata2_len}')
        xdata2_max = xdata2_len / 2
        xdata2_min = -xdata2_max
        xdata2_max -= 1

        self.plot1.axis([xmin1, xmax1, ymin1, ymax1])
        self.plot1line1.set_data(xdata1, np.real(ydata1))
        self.plot1line1.set_data(xdata1, np.imag(ydata1))
        self.plot1line2.set_data(xdata1, ydata1)

        self.plot2.axis([xdata2_min, xdata2_max, ymin2, ymax2])
        self.plot2line.set_data(np.linspace(xdata2_min, xdata2_max, xdata2_len), ydata2)

        plt.draw()


def main():
    mp = MyPlot()
    numsamples = 4096
    numcycles = 750
    realshow = 100
    xdata = np.linspace(0, np.pi*2*numcycles, numsamples)
    ydata1 = np.exp(1j*xdata)
    ydata2 = np.abs(np.fft.fftshift(np.fft.fft(ydata1))) / numsamples
    mp.plot(xdata[0:realshow], xdata, ydata1[0:realshow], ydata2)
    plt.pause(100)
    return 0


if __name__ == '__main__':
    exit(main())
