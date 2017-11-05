#!/bin/sh

set -e -x

PFKARCH=$( sh ../scripts/architecture )
export PFKARCH

if [ ! -d mscgen ] ; then
    echo 'no mscgen dir, skipping'
    exit 0
fi

cd "$OBJDIR/mscgen"
make install

cd $HOME/pfk/$PFKARCH/mscgen-0.20/bin
links=$( echo * )

cd $HOME/pfk/$PFKARCH/bin

for f in $links ; do
    rm -f $f
    ln -s ../mscgen-0.20/bin/$f
done

exit 0
