
import argparse, copy, datetime, hashlib, logging

#from matplotlib import pyplot as plt
#%matplotlib inline
#import matplotlib

import numpy as np
import pandas as pd
pd.options.display.max_rows=160
pd.options.display.max_columns=32
pd.options.display.max_colwidth=128
#from IPython.display import display, HTML
import seaborn as sns
import os, re, statistics, sys, time, warnings
from scipy import interpolate
import csv
import importlib

if '<package>' in locals():
    print("attempting reload of <package> library")
    importlib.reload(<package>)
else:
    print("importing resid library first time")
    from <dir>  import <file>  as <package>


print("got <package> version %u" % (<package>.version()))


df = pd.DataFrame(columns=['num','val'])

df=df.append({'num':1,'val':6},ignore_index=True)
df=df.append({'num':2,'val':7},ignore_index=True)
df=df.append({'num':3,'val':8},ignore_index=True)
df=df.append({'num':4,'val':9},ignore_index=True)
df=df.append({'num':5,'val':0},ignore_index=True)

df.insert(df.shape[1],'iq',[[] for _ in range(df.shape[0])])
ind=df.loc[df.num==3].index[0]
df.at[ind,'iq'] = [1,2,3,4]

def offset_by_hz(iq, hz=7500.):
    t = np.arange(0, iq.shape[0], dtype=np.float64)
    return iq * np.exp(-1j * 2 * np.pi * t * hz / 30720000.0)



if added_awgn_std_dev is not None:
    raw_iq_data = raw_iq_data + \
        my_utils.get_zero_mean_complex_noise_samples(
            raw_iq_data.shape[0], added_awgn_std_dev,
            seed=sample_id)


def extract_and_cfo_symbol (corr_start_pos, cfo, ideal_data, corr_size=100):
    corr_start_pos = int(corr_start_pos - (corr_size / 2))
    corr_range = 2048 + corr_size
    # reference to symbol iq data over a range for correlation
    sym_data = filtered_iq_data[corr_start_pos : corr_start_pos + corr_range]
    # correct the frequency offset from the database
    sym_adj = offset_by_hz(sym_data,cfo)
    # correlate the ideal symbol to the signal,
    corr = np.correlate(sym_adj, ideal_data)
    # first to see where the symbol is, and
    argmax_pos = np.argmax(np.abs(corr))
    # second to see what phase it's in.
    argmax = corr[argmax_pos]
    rot = np.exp(-1j * np.angle(argmax))

    # extract the symbols, rotating them into phase with the ideal.
    # NOTE we're grabbing 2050, not 2048, so the interpolation
    # stuff below can work.
    sym_data = sym_adj[argmax_pos-1:argmax_pos+2049] * rot

    # scale the signal to match the ideal's magnitudes.
    sym_normalizer = np.average(np.abs(sym_data)) / np.average(np.abs(ideal_data))
    sym_data /= sym_normalizer
    if (verbose):
        print("normalization factor %f" % (sym_normalizer))

    return sym_data

    def interpolate_search (symnum,sym_data,ideal_data):

        def interpolate_subtract (ratio):
        
            inds = np.arange(0,2050)
            newinds = inds + ratio
            
            if (do_interpolations == 1):
                interpdata = np.interp(newinds,inds,sym_data)
            elif (do_interpolations == 2):
                # scipy's b-spline code doesn't support complex numbers,
                # so interpolate the real and imaginary components separately
                # and recombine them.
                tcki = interpolate.splrep(inds,np.real(sym_data),s=0)
                tckq = interpolate.splrep(inds,np.imag(sym_data),s=0)
                interpdata = interpolate.splev(newinds, tcki, der=0) + \
                             interpolate.splev(newinds, tckq, der=0) * 1j

            # re-correlate because interpolation rotates the data.
            # this should be a faster correlate, since the interpdata
            # is only 3 longer than the ideal_data. also sometimes,
            # the ideal sample number moves left or right by 1 sample...
            corr = np.correlate(interpdata,ideal_data)
            corrpos = np.argmax(np.abs(corr))
            rot = np.exp(-1j*np.angle(corr[corrpos]))
            interpdata *= rot
            resids = interpdata[corrpos:2048+corrpos] - ideal_data
            stddev = np.std(resids)
            return resids, stddev

        smallest_stdev = 99999999.
        num_interps = 21
        best_ratio = 0.
        
        if (verbose):
            print("interp %u: " % symnum,end='')
        
        for ratio in np.linspace(-1.,1.,num_interps):
            resids, stddev = interpolate_subtract(ratio)
            if (verbose):
                print("%.2f,%.0f " % (ratio,stddev),end='')
            if (stddev < smallest_stdev):
                smallest_stdev = stddev
                best_resids = resids.copy()
                best_ratio = ratio

        if (verbose):
            print("BEST RATIO:%.2f" % best_ratio)
        return best_resids

    resids3  = interpolate_search( 3, sym3_data,  ideal_3_iq_data )
    resids10 = interpolate_search(10, sym10_data, ideal_10_iq_data)


    query = "SELECT stuff     " + \
            " FROM dbproject.tablename AS t" + \
            " WHERE t.id = %u" % (value)
    result = db.execute_query(query)
    if len(result)==1 and len(result[0])==24:
        return {
            'blah'      :       result[0][ 0],
            'blah'      :       result[0][ 1],
            'floatblah' : float(result[0][ 2]),
            'floatmaybenullblah' : 0. if result[0][ 6]==None else float(result[0][ 6]),
        }
    return None

    corr = np.correlate(interpdata,ideal_data)
    corrpos = np.argmax(np.abs(corr))
    rot = np.exp(-1j*np.angle(corr[corrpos]))
    interpdata *= rot
    resids = interpdata[corrpos:2048+corrpos] - ideal_data
    resids[2038:2048] = 0j

    resid3_fft = np.fft.fftshift(np.fft.fft(resids3))

    with open(csvfile,'w',newline='') as f:
        wr = csv.writer(f,delimiter=' ')
        for csvind in range(csvlines):
            wr.writerow([csvind,
                         sym3_data[csvind+1].real, sym3_data[csvind+1].imag,
                         ideal_3_iq_data[csvind].real, ideal_3_iq_data[csvind].imag,
                         resids3[csvind].real, resids3[csvind].imag])
        f.close()

    plotcmds.append("splot " + \
            "'corr%u.csv' using 1:2:3 with lines title 'raw iq'  , " % (sample_id) + \
            "'corr%u.csv' using 1:4:5 with lines title 'ideal'   , " % (sample_id) + \
            "'corr%u.csv' using 1:6:7 with lines title 'residual', " % (sample_id) + \
            "'corr%u.csv' using ($1):(-2000):(-2000) with lines title 'resid axis'\n" % (sample_id))

    if (normalize):
        mean = np.mean(resids3)
        resids3 -= mean
        stddev = np.std(resids3)
        resids3 /= stddev

    return np.real(resids3), np.imag(resids3), np.real(resids10), np.imag(resids10)
