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

cd rxvt-unicode
if [ ! -f 00-PFK-CONFIGURE ] ; then
    echo 'no rxvt-unicode configure file, correct branch?'
    # this is an error, check out the proper branch.
    exit 1
fi

RXVT_DIR="$PWD"

mkdir -p "$OBJDIR/urxvt"
cd "$OBJDIR/urxvt"

if [ ! -f Makefile ] ; then
    "$RXVT_DIR/configure" --prefix=$HOME/pfk/$PFKARCH/urxvt-9.22 \
		--enable-xft --enable-font-styles --disable-perl \
		--disable-iso14755 --enable-selectionscrolling \
		--enable-mousewheel --disable-utmp --disable-wtmp \
		--disable-lastlog --with-term=vt100
    cp src/Makefile src/Makefile.orig
    sed -e s/gcc/g++/ < src/Makefile.orig > src/Makefile
fi

make $PFK_CONFIG_contrib_makejobs

exit 0
