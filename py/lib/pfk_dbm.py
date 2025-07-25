#!/usr/bin/env python3

import math

def dbm_to_rms(dbm: float,
               impedance: float = 50,
               verbose: bool = False) -> float:
    """convert power in dBm to a voltage level, using the
    specified impedance (defaults to 50 ohm)."""
    # first convert to milliwatts
    watts = math.pow(10, dbm / 10) / 1000.0
    volts = math.sqrt(watts * impedance)
    if verbose:
        print(f'dbm = {dbm:.2f}  watts = {watts:.2f}  rms = {volts:.2f}')
    return volts

def rms_to_p2p(rms: float,
               verbose: bool = False) -> float:
    p2p = rms * 2 * math.sqrt(2)
    if verbose:
        print(f'rms = {rms:.2f}  p2p = {p2p:.2f}')
    return p2p

def rms_to_dbm(rms: float,
               impedance: float = 50,
               verbose: bool = False) -> float:
    watts = math.pow(rms, 2) / impedance
    dbm = 10 * math.log10(watts * 1000)
    if verbose:
        print(f'rms = {rms:.2f}  watts = {watts:.2f}  dbm = {dbm:.2f}')
    return dbm

def main():
    dbm = 13.0103
    rms = dbm_to_rms(dbm, 50, True)
    dbm = rms_to_dbm(rms, 50, True)
    p2p = rms_to_p2p(rms, True)

if __name__ == '__main__':
    exit(main())
