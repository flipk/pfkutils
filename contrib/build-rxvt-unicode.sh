#!/bin/sh

set -e -x

PFKARCH=$( sh ../scripts/architecture )
export PFKARCH

if [ ! -d rxvt-unicode ] ; then
    echo 'no rxvt-unicode dir, skipping rxvt build'
    # i'm not going to consider this an error, maybe
    # i just didn't extract it.
    exit 0
fi

cd rxvt-unicode
if [ ! -f 00-PFK-CONFIGURE ] ; then
    echo 'no rxvt-unicode configure file, correct branch?'
    # this is an error, check out the proper branch.
    exit 1
fi

if [ ! -f Makefile ] ; then
    sh 00-PFK-CONFIGURE
fi

make -j3

exit 0
