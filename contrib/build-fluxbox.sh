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

case x$PFK_CONFIG_flubox_xinerama in
    xenable)
	xinerama=--enable-xinerama
	;;
    xdisable)
	xinerama=--disable-xinerama
	;;
    xdefault)
	xinerama=""
	;;
    *)
	echo please set PFK_CONFIG_flubox_xinerama to yes or no in config
	exit 1
	;;
esac

FLUXBOX_DIR="$PWD"
mkdir -p "$OBJDIR/fluxbox"
cd "$OBJDIR/fluxbox"

if [ ! -f Makefile ] ; then
    "$FLUXBOX_DIR/configure" --prefix=$HOME/pfk/$PFKARCH/fluxbox_1.3.7 \
		--disable-xrandr $xinerama
fi

make $PFK_CONFIG_contrib_makejobs

exit 0
