#!/usr/bin/env python3

import os

sd_driver_base = '/sys/bus/scsi/drivers/sd'

class ScsiDevice:
    syspath: str  # e.g. "/sys/bus/scsi/drivers/sd/9:0:0:0/"
    blockdev: str  # e.g. "sda" / "sdg"
    provmodefile: str
    provmode: str
    ok: bool

    def __init__(self, syspath: str, blockdev: str):
        self.ok = False
        self.syspath = syspath
        self.blockdev = blockdev
        de: os.DirEntry
        for de in os.scandir(syspath + '/scsi_disk'):
            provmodefile = syspath + '/scsi_disk/' + de.name + '/provisioning_mode'
            if os.path.exists(provmodefile):
                self.provmodefile = provmodefile
                with open(provmodefile, "r") as pmfd:
                    dat = pmfd.read()
                    self.provmode = dat.strip()
                self.ok = True

    def __str__(self):
        return f'ScsiDevice({self.syspath}, {self.blockdev}, {self.provmode})'


class ScsiDevices:
    devs: list[ScsiDevice]

    def __init__(self):
        self.devs = []
        de: os.DirEntry
        for de in os.scandir(sd_driver_base):
            if de.name[0].isdigit():
                syspath = sd_driver_base + '/' + de.name
                blockpath = syspath + '/block'
                if os.path.exists(blockpath):
                    de2: os.DirEntry
                    for de2 in os.scandir(blockpath):
                        blockdev = de2.name
                        sd = ScsiDevice(syspath, blockdev)
                        if sd.ok:
                            self.devs.append(sd)

    def find_device(self, name: str) -> ScsiDevice | None:  # e.g. "sdc"
        for d in self.devs:
            if d.blockdev == name:
                return d
        return None

    def __str__(self):
        ret = ''
        for d in self.devs:
            ret += d.__str__() + '\n'
        return ret


def main():
    sds = ScsiDevices()
    print(sds)


if __name__ == '__main__':
    main()
