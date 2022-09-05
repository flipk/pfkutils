#!/usr/bin/env python3

#     sys.path.append('/code/feature_blahblah/python/')
#     then import

import os
import sys


debug_enable: bool = False

if __name__ == '__main__':
    # NOTE if you get
    #    "UnicodeEncodeError: 'ascii' codec can't encode character"
    # use:   export PYTHONIOENCODING=utf-8

    # this does not seem to work (too late?)
    os.environ["PYTHONIOENCODING"] = "utf-8"

    which = 0  # main
    for arg in sys.argv:
        if arg == '-d':
            debug_enable = True
        elif arg == '-t1':
            which = 1  # main_test1
        elif arg == '-t2':
            which = 2  # main_test2

    if which == 0:
        main()
    elif which == 1:
        main_test1()
    elif which == 2:
        main_test2()
