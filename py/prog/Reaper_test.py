#!/usr/bin/env python3

import sys
import re
import time
import subprocess
from Reaper import Reaper

_match_pattern = re.compile('oosplash|soffice.bin')


def _testmatch(name: str) -> bool:
    m = _match_pattern.search(name)
    if m:
        return True
    return False


def main(argv: list[str]):
    if len(argv) != 1:
        print(f'usage: Reaper_test.py <process_to_run>')
        return 1
    r = Reaper()
    cmd = [argv[0]]
    cp = subprocess.run(cmd, check=False,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT)
    print(f'cp = {cp}')
    time.sleep(5)
    r.cleanup_children(_testmatch)
    time.sleep(5)


if __name__ == '__main__':
    exit(main(sys.argv))
