#!/usr/bin/env python3

import math


def p2p_to_rms(p2p: float) -> float:
    """convert peak-to-peak voltage to RMS"""
    return p2p / (2*math.sqrt(2))


def rms_to_p2p(rms: float) -> float:
    """convert RMS to peak-to-peak"""
    return rms * 2 * math.sqrt(2)


def dbm_to(dbm: float,
           impedance: float = 50) -> (float, float, float):
    """takes dBm and impedance, returns mW, rms, and p2p"""
    mw = math.pow(10, dbm / 10)
    watts = mw / 1000
    rms = math.sqrt(watts * impedance)
    p2p = rms_to_p2p(rms)
    return mw, rms, p2p


def to_dbm(mw: float | None = None,
           rms: float | None = None,
           p2p: float | None = None,
           impedance: float = 50) -> float:
    """takes watts, or rms, or p2p, and impedance, and returns dBm"""
    if p2p:
        rms = p2p_to_rms(p2p)
    if rms:
        mw = math.pow(rms, 2) / impedance * 1000
    dbm = 10 * math.log10(mw)
    return dbm


def main():
    print('expect: dBm = 13.0103  --> mW = 20.0  rms = 1.0  p2p = 2.8284')
    dbm = 13.0103
    mw, rms, p2p = dbm_to(dbm)
    print(f' 1:     dBm = {dbm:.4f}  --> mW = {mw:.1f} '
          f' rms = {rms:.1f}  p2p = {p2p:.4f}')
    dbm = to_dbm(mw=mw)
    print(f' 2:     dBm = {dbm:.4f}  <-- mW = {mw:.1f}  ')
    dbm = to_dbm(rms=rms)
    print(f' 3:     dBm = {dbm:.4f}  <--            rms = {rms:.1f}  ')
    dbm = to_dbm(p2p=p2p)
    print(f' 4:     dBm = {dbm:.4f}  <--            '
          f'           p2p = {p2p:.4f}  ')


if __name__ == '__main__':
    exit(main())
