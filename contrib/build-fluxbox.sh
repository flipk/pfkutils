#!/bin/sh

set -e -x

PFKARCH=$( architecture )
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

if [ ! -f $OBJDIR/fluxbox/Makefile ] ; then
    touch Makefile.in aclocal.m4 build-aux/compile build-aux/config.guess \
	  build-aux/config.sub build-aux/depcomp build-aux/install-sh \
	  build-aux/missing config.h.in configure nls/C/Makefile.in \
	  nls/be_BY/Makefile.in nls/bg_BG/Makefile.in nls/cs_CZ/Makefile.in \
	  nls/da_DK/Makefile.in nls/de_AT/Makefile.in nls/de_CH/Makefile.in \
	  nls/de_DE/Makefile.in nls/el_GR/Makefile.in nls/en_GB/Makefile.in \
	  nls/en_US/Makefile.in nls/es_AR/Makefile.in nls/es_ES/Makefile.in \
	  nls/et_EE/Makefile.in nls/fi_FI/Makefile.in nls/fr_CH/Makefile.in \
	  nls/fr_FR/Makefile.in nls/he_IL/Makefile.in nls/it_IT/Makefile.in \
	  nls/ja_JP/Makefile.in nls/ko_KR/Makefile.in nls/lv_LV/Makefile.in \
	  nls/mk_MK/Makefile.in nls/nb_NO/Makefile.in nls/nl_NL/Makefile.in \
	  nls/no_NO/Makefile.in nls/pl_PL/Makefile.in nls/pt_BR/Makefile.in \
	  nls/pt_PT/Makefile.in nls/ru_RU/Makefile.in nls/sk_SK/Makefile.in \
	  nls/sl_SI/Makefile.in nls/sv_SE/Makefile.in nls/tr_TR/Makefile.in \
	  nls/uk_UA/Makefile.in nls/vi_VN/Makefile.in nls/zh_CN/Makefile.in \
	  nls/zh_TW/Makefile.in
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
