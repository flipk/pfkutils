#!/usr/bin/env python3

import numpy as np
from scipy.optimize import curve_fit

dataset = 2

if dataset == 1:
    x_data = np.array([1.0, 2.0, 3.0, 4.0, 5.0,
                       6.0, 7.0, 8.0, 9.0, 10.0,
                       11.0, 12.0, 13.0])
    y_data = np.array([24.8, 38.4, 48.0, 54.4, 60.6,
                       65.6, 69.6, 72.8, 76.0, 78.4,
                       81.2, 83.2, 85.6])

if dataset == 2:
    x_data = np.array([ 1.0,  2.0,  3.0,  4.0,   5.0,
                        6.0,  7.0,  8.0,  9.0,  10.0,
                        11.0, 12.0, 13.0, 14.0, 15.0,
                        16.0, 17.0, 18.0, 19.0, 20.0,
                        21.0, 22.0, 23.0, 24.0, 25.0,
                        26.0 ])
    y_data = np.array([28.0, 40.8, 48.8, 55.2, 61.6,
                       66.4, 70.4, 73.6, 76.8, 79.2,
                       81.6, 84.0, 85.6, 87.2, 88.8, 
                       89.6, 91.2, 92.0, 92.8, 93.6, 
                       94.4, 95.2, 96.0, 96.3, 96.8, 
                       97.6])

def func(x, m, p, b):
    return m * np.power(x, p) + b

popt, pcov = curve_fit(func, x_data, y_data,
                       p0=(1000, 0.02, -1000), maxfev=20000)

m, p, b = popt

print(f'm = {m:.20e}')
print(f'p = {p:.20e}')
print(f'b = {b:.20e}')
