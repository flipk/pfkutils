#!/bin/sh

PFKARCH=$( sh ../scripts/architecture )
export PFKARCH

if [ ! -d mscgen ] ; then
    echo 'no mscgen dir, skipping'
    exit 0
fi

MSCGEN_DIR="$PWD/mscgen"
cd "$MSCGEN_DIR"
dirs=$( find . -type d | sort )
files=$( find . -type f | sort )

if [ ! -f $OBJDIR/mscgen/Makefile ] ; then
    touch config.h.in configure aclocal.m4 ./man/Makefile.in \
	  ./src/Makefile.in ./Makefile.in ./test/Makefile.in \
	  ./examples/Makefile.in
fi

echo making linktree

mkdir -p "$OBJDIR/mscgen"
cd "$OBJDIR/mscgen"
if [ ! -f SYMLINKS_MAKE ] ; then
    mkdir -p $dirs
    for f in $files ; do
	rm -f "$f"
	ln -s "$MSCGEN_DIR/$f" "$f"
    done
    touch SYMLINKS_MAKE
fi

set -e -x

if [ ! -f Makefile ] ; then
    ./configure --prefix=$HOME/pfk/$PFKARCH/mscgen-0.20
fi

make $PFK_CONFIG_contrib_makejobs

exit 0
