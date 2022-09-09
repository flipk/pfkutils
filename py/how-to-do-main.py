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

    # this does not work, too late, but interesting anyway.
    os.environ["PYTHONIOENCODING"] = "utf-8"

    version = sys.version_info.major + (sys.version_info.minor / 10.0)
    if version >= 3.7:
        sys.stdout.reconfigure(encoding='utf-8')
    else:
        # reconfigure method not supported, so instead:
        sys.stdout = open(sys.stdout.fileno(), mode='w',
                          encoding='utf8', buffering=1)

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
