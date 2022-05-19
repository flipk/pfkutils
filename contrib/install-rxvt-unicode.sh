#!/bin/sh

set -e -x

PFKARCH=$( architecture )
export PFKARCH

if [ ! -d rxvt-unicode ] ; then
    echo 'no rxvt-unicode dir, skipping rxvt build'
    # i'm not going to consider this an error, maybe
    # i just didn't extract it.
    exit 0
fi

cd "$OBJDIR/urxvt"

make install

cd $HOME/pfk/$PFKARCH/urxvt-9.22/bin
links=$( echo * )
cd $HOME/pfk/$PFKARCH/bin

for f in $links ; do
    rm -f $f
    ln -s ../urxvt-9.22/bin/$f
done

exit 0
