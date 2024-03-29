#!/usr/bin/env python3
import os
import re
import struct
from crc32 import crc32


class UbootEnv:
    envlen: int
    encoded: bytearray
    variables: dict
    _varpatt: re.Pattern

    def __init__(self, envlen: int = 16384):
        self.envlen = envlen
        self.init()
        self._varpatt = re.compile(b'^([^=]+)=(.*)$')

    def init(self):
        self.variables = {}
        self.encoded = bytearray(self.envlen)

    def load_env(self, fname: str, offset: int = 0) -> bool:
        try:
            with open(fname, mode='rb') as fd:
                fd.seek(offset)
                self.encoded[0:self.envlen] = fd.read(self.envlen)
            return True
        except Exception as e:
            print(f'ERROR failed to read file {fname}: {e}')
            return False

    def save_env(self, fname: str, offset: int = 0) -> bool:
        try:
            with open(fname, mode='wb') as fd:
                fd.seek(offset)
                fd.write(self.encoded)
            return True
        except Exception as e:
            print(f'ERROR failed to write file {fname}: {e}')
            return False

    def decode_env(self) -> bool:
        """take in a binary encoding of uboot env vars, validate the CRC,
        and decode the variables in it, returning a dict."""
        crcval = crc32(0xffffffff, self.encoded[4:]) ^ 0xffffffff
        origcrc = struct.unpack('<I', self.encoded[0:4])[0]
        res = crcval ^ origcrc
        # print(f'crc {crcval:08x} ^ orig {origcrc:08x} = {res:08x}')
        self.variables = {}
        if res == 0:
            for v in self.encoded[4:].split(b'\x00'):
                if len(v) > 0:
                    m = self._varpatt.search(v)
                    if m:
                        self.variables[m.group(1).decode()] = m.group(2).decode()
            return True
        return False

    def encode_env(self) -> bool:
        """take a dict containing uboot environment variables, encoded them
        into binary format, and add the appropriate CRC."""
        self.encoded = bytearray(self.envlen)
        pos = 4  # leave room for CRC at the front.
        for k in self.variables.keys():
            varbin = (k + '=' + self.variables[k]).encode() + b'\x00'
            vslen = len(varbin)
            endpos = pos + vslen
            if endpos > self.envlen:
                return False
            self.encoded[pos:endpos] = varbin
            pos = endpos
        crcval = crc32(0xffffffff, self.encoded[4:]) ^ 0xffffffff
        # print(f'crc = {crcval:08x}')
        struct.pack_into('<I', self.encoded, 0, crcval)
        return True

    def get(self, varname: str) -> str | None:
        return self.variables.get(varname, None)

    def set(self, varname: str, value: str):
        self.variables[varname] = value

    def erase(self, varname: str):
        try:
            del self.variables[varname]
        except KeyError:
            pass

    def test(self) -> int:
        self.set('bootcmd', 'this is the boot cmd variable')
        self.set('bootdev', 'mmc2')
        self.set('testvar', 'if mmc_init; then load file; fi')
        if not self.encode_env():
            print('ERROR encoding: not enough space?')
            return 1
        printlen = 128
        print(f'encoded[0:{printlen}] = {self.encoded[0:printlen]}')
        if not self.save_env('testfile'):
            print("'ERROR encoding: can't save'")
            return 1
        self.init()
        if not self.load_env('testfile'):
            print("'ERROR encoding: can't save'")
            return 1
        os.unlink('testfile')
        if self.decode_env():
            print('decoded:')
            for k in self.variables:
                print(f"  '{k}': '{self.variables[k]}',")
        else:
            print('decode CRC failed!')
            return 1
        return 0


if __name__ == '__main__':
    ue = UbootEnv()
    exit(ue.test())
