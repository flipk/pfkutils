#!/usr/bin/env python3

import sys
import math
from enum import Enum


def vpp_to_vrms(vpp: float) -> float:
    """convert peak-to-peak voltage to RMS"""
    return vpp / (2*math.sqrt(2))


def vrms_to_vpp(vrms: float) -> float:
    """convert RMS to peak-to-peak"""
    return vrms * 2 * math.sqrt(2)


def dbm_to(dbm: float,
           impedance: float = 50) -> (float, float, float):
    """takes dBm and impedance, returns mW, vrms, and vpp"""
    mw = math.pow(10, dbm / 10)
    watts = mw / 1000
    vrms = math.sqrt(watts * impedance)
    vpp = vrms_to_vpp(vrms)
    return mw, vrms, vpp


def to_dbm(mw: float | None = None,
           vrms: float | None = None,
           vpp: float | None = None,
           impedance: float = 50) -> float:
    """takes watts, or vrms, or vpp, and impedance, and returns dBm"""
    if vpp:
        vrms = vpp_to_vrms(vpp)
    if vrms:
        mw = math.pow(vrms, 2) / impedance * 1000
    dbm = 10 * math.log10(mw)
    return dbm


def self_test():
    print('expect: dBm = 13.0103  --> mW = 20.0  vrms = 1.0  vpp = 2.8284')
    dbm = 13.0103
    mw, vrms, vpp = dbm_to(dbm)
    print(f' 1:     dBm = {dbm:.4f}  --> mW = {mw:.1f} '
          f' vrms = {vrms:.1f}  vpp = {vpp:.4f}')
    dbm = to_dbm(mw=mw)
    print(f' 2:     dBm = {dbm:.4f}  <-- mW = {mw:.1f}  ')
    dbm = to_dbm(vrms=vrms)
    print(f' 3:     dBm = {dbm:.4f}  <--            vrms = {vrms:.1f}  ')
    dbm = to_dbm(vpp=vpp)
    print(f' 4:     dBm = {dbm:.4f}  <--            '
          f'            vpp = {vpp:.4f}  ')


def usage():
    print('\n'
          'usage: pfk_dbm dbm <dbm> [imp <impedance>]\n'
          '       pfk_dbm vpp <vpp>  [imp <impedance>]\n'
          '       pfk_dbm vrms <vrms>  [imp <impedance>]\n'
          '       pfk_dbm w <w> [imp <impedance>]\n'
          '       pfk_dbm mw <mw> [imp <impedance>]\n'
          ' in all cases, takes specified arg and returns all others\n'
          '\n'
          '       pfk_dbm test   [performs self-test]\n'
          ' if impedance not specified, assumes 50 ohms\n'
          )

class Op(Enum):
    OP_NONE = 0
    OP_DBM = 1
    OP_VPP = 2
    OP_VRMS = 3
    OP_MW = 4
    OP_TEST = 5

class Args:
    ok: bool
    op: Op
    impedance: float
    dbm: float
    vpp: float
    vrms: float
    mw: float

    def __init__(self):
        self.ok = False
        self.impedance = 50
        self.op = Op.OP_NONE

    def parse(self, args: list[str]) -> bool:
        if len(args) < 2:
            return False
        if args[1] == 'test':
            self.op = Op.OP_TEST
            return True
        if len(args) < 3:
            return False
        if args[1] == 'dbm':
            self.op = Op.OP_DBM
            self.dbm = float(args[2])
        elif args[1] == 'vpp':
            self.op = Op.OP_VPP
            self.vpp = float(args[2])
        elif args[1] == 'vrms':
            self.op = Op.OP_VRMS
            self.vrms = float(args[2])
        elif args[1] == 'w':
            self.op = Op.OP_MW
            self.mw = float(args[2]) * 1000.0
        elif args[1] == 'mw':
            self.op = Op.OP_MW
            self.mw = float(args[2])
        else:
            return False
        if len(args) == 3:
            return True
        if len(args) == 5:
            if args[3] == 'imp':
                self.impedance = float(args[4])
            else:
                return False
            return True
        return False

    def __str__(self):
        return \
            f'dBm = {self.dbm:.2f}  ' \
            f'mW = {self.mw:.6f}  ' \
            f'vrms = {self.vrms:.6f}  ' \
            f'vpp = {self.vpp:.6f}'

def main(argv):
    args = Args()
    if not args.parse(argv):
        usage()
        return 1
    if args.op == Op.OP_TEST:
        self_test()
        return 0
    elif args.op == Op.OP_VPP:
        args.dbm = to_dbm(vpp=args.vpp)
        args.mw, args.vrms, args.vpp = dbm_to(args.dbm, args.impedance)
    elif args.op == Op.OP_VRMS:
        args.dbm = to_dbm(vrms=args.vrms)
        args.mw, args.vrms, args.vpp = dbm_to(args.dbm, args.impedance)
    elif args.op == Op.OP_MW:
        args.dbm = to_dbm(args.mw)
        args.mw, args.vrms, args.vpp = dbm_to(args.dbm, args.impedance)
    elif args.op == Op.OP_DBM:
        args.mw, args.vrms, args.vpp = dbm_to(args.dbm, args.impedance)
    print(args)

if __name__ == '__main__':
    exit(main(sys.argv))
