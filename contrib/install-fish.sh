#!/bin/sh

set -e -x

PFKARCH=$( architecture )
export PFKARCH

if [ ! -d fish-shell ] ; then
    echo 'no fish-shell dir, skipping fish build'
    exit 0
fi

cd "$OBJDIR/fish"
make install

cd $HOME/pfk/$PFKARCH/fish-2.6.0/bin
links=$( echo * )

cd $HOME/pfk/$PFKARCH/bin

for f in $links ; do
    rm -f $f
    ln -s ../fish-2.6.0/bin/$f
done

exit 0
