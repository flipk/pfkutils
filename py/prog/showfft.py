#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
from matplotlib import gridspec
from matplotlib.axes import Axes
from matplotlib.figure import Figure
from matplotlib.gridspec import GridSpec
from matplotlib.lines import Line2D


class MyPlot:
    xmin: int
    xmax: int
    ymin: int
    ymax: int
    xrange: int
    yrange: int
    fig0: Figure
    gs: GridSpec
    fig1: Figure
    plot1: Axes
    plot1line: Line2D

    def __init__(self):
        # do not force-raise window on EVERY SINGLE REPLOT.
        # why would ANYONE want that????
        plt.rcParams["figure.raise_window"] = False
        self.fig0 = plt.figure(figsize=(10, 10), dpi=80)
        plt.ion()
        plt.show()  # needed?
        self.fig1 = plt.figure(1)
        self.gs = gridspec.GridSpec(ncols=1, nrows=1,
                                    width_ratios=[1], height_ratios=[1])
        self.plot1 = self.fig1.add_subplot(self.gs[0])

        self.xmin = -400
        self.xmax = 400
        self.ymin = 0
        self.ymax = 1
        self.plot1.axis([self.xmin, self.xmax, self.ymin, self.ymax])
        self.plot1.set_xlabel(f'frequency')
        self.plot1.set_ylabel('magnitude')
        self.plot1line, = self.plot1.plot([], [])

    def plot(self, xdata, ydata):
        self.xmin = np.min(xdata)
        self.xmax = np.max(xdata)
        self.ymin = np.min(ydata)
        self.ymax = np.max(ydata)

        self.xrange = self.xmax - self.xmin
        self.yrange = self.ymax - self.ymin

        self.xmin -= self.xrange / 10
        self.xmax += self.xrange / 10
        self.ymin -= self.yrange / 10
        self.ymax += self.yrange / 10

        self.plot1.axis([self.xmin, self.xmax, self.ymin, self.ymax])
        self.plot1line.set_data(xdata, ydata)
        plt.draw()


def main():
    mp = MyPlot()
    xdata = np.linspace(-np.pi*1024, np.pi*1024, 4096)
    ydata = np.exp(1j*xdata)
    ydata = np.abs(np.fft.fftshift(np.fft.fft(ydata))) / 4096
    mp.plot(xdata, ydata)
    plt.pause(100)
    return 0


if __name__ == '__main__':
    exit(main())
