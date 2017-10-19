#!/bin/sh

PFKARCH=$( sh ../scripts/architecture )
export PFKARCH

if [ ! -d fish-shell ] ; then
    echo 'no fish-shell dir, skipping fish build'
    exit 0
fi

FISH_DIR="$PWD/fish-shell"
cd "$FISH_DIR"
dirs=$( find . -type d | sort )
files=$( find . -type f | sort )

echo making linktree

mkdir -p "$OBJDIR/fish"
cd "$OBJDIR/fish"
if [ ! -f SYMLINKS_MAKE ] ; then
    mkdir -p $dirs
    for f in $files ; do
	rm -f "$f"
	ln -s "$FISH_DIR/$f" "$f"
    done
    touch SYMLINKS_MAKE
fi

set -e -x

if [ ! -f Makefile ] ; then
    ./configure --prefix=$HOME/pfk/$PFKARCH/fish-2.6.0
fi

make $PFK_CONFIG_contrib_makejobs

exit 0
