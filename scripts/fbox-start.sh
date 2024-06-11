#!/bin/bash

# these are supposed to be set before we get here.
if [[ -z "$FLUXBOX_SETTINGS" ]] ; then
    echo WARNING: FLUXBOX_SETTINGS was supposed to be set before this
    export FLUXBOX_SETTINGS=$TMP/.fbsettings.$$
fi
if [[ -z "$FLUXBOX_INIT_FILE" ]] ; then
    echo WARNING: FLUXBOX_INIT_FILE was supposed to be set before this
    export FLUXBOX_INIT_FILE=$TMP/.fbinit.$$
fi

# the $FLUXBOX_SETTINGS file should have two fields:
#   <alt or win> <console or vnc>

# the "init.in" file has these patterns:
#   @@keysconfig@@ -> ALT or WIN
#   @@keysfile@@ -> keys.alt or keys.win
#   @@menufile@@ -> console or vnc

if [[ -f "$FLUXBOX_SETTINGS" ]] ; then
    read keysconfig menuconfig < "$FLUXBOX_SETTINGS"
fi

case $keysconfig in
    alt)
        KEYS_CONFIG=ALT
        KEYS_FILE=keys.alt
        ;;
    win)
        KEYS_CONFIG=WIN
        KEYS_FILE=keys.win
        ;;
    *)
        # default.
        KEYS_CONFIG=ALT
        KEYS_FILE=keys.alt
        ;;
esac

case $menuconfig in
    console)
        MENU_FILE=menu.console
        ;;
    vnc)
        MENU_FILE=menu.vnc
        ;;
    *)
        # default
        MENU_FILE=menu.console
esac

if [[ -z "$FLUXBOX_INIT_FILE" ]] ; then
    export FLUXBOX_INIT_FILE=$TMP/fluxbox-init.$$
fi

sed -e s,@@keysconfig@@,${KEYS_CONFIG},g \
    -e s,@@keysfile@@,${KEYS_FILE},g \
    -e s,@@menufile@@,${MENU_FILE},g \
    < ~/pfk/etc/fluxbox/init.in > "$FLUXBOX_INIT_FILE"

exec $HOME/pfk/$PFKARCH/bin/fluxbox -rc "$FLUXBOX_INIT_FILE"
