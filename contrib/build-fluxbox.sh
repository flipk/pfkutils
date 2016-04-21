#!/bin/sh

set -e -x

PFKARCH=$( sh ../scripts/architecture )
export PFKARCH

if [ ! -d fluxbox ] ; then
    echo 'no fluxbox dir, skipping fluxbox build'
    # i'm not going to consider this an error, maybe
    # i just didn't extract it.
    exit 0
fi

cd fluxbox
if [ ! -f 00-PFK-CONFIGURE ] ; then
    echo 'no fluxbox configure file, correct branch?'
    # this is an error, check out the proper branch.
    exit 1
fi

if [ ! -d m4 ] ; then
    mkdir m4
fi

if [ ! -f configure ] ; then
    autoreconf -i
fi

if [ ! -f Makefile ] ; then
    sh 00-PFK-CONFIGURE
fi

make -j3

exit 0
