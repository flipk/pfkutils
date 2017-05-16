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

cd "$OBJDIR/fluxbox"
make install

cd $HOME/pfk/$PFKARCH/fluxbox_1.3.7/bin
links=$( echo * )
cd $HOME/pfk/$PFKARCH/bin

for f in $links ; do
    rm -f $f
    ln -s ../fluxbox_1.3.7/bin/$f
done

exit 0
