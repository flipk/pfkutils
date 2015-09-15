#!/bin/sh

# default bg if no image file can be found.
xsetroot -solid \#102030

bg=$HOME/.background.png
# one of the following will succeed and the others
# will make an error msg. redirect the errors to null
if [ -f $bg ] ; then
    exec >/dev/null 2>/dev/null
    # if feh is installed, use that.
    feh --bg-tile $bg
    # if xsri is installed, use that.
    xsri --tile=$bg
    # maybe 'display' is installed? gross.
    rootwin=`xdpyinfo | awk '/root window id/ { print $4 }'`
    display -window $rootwin $bg
fi

if [ x$1 = xwait ] ; then
    sleep 3600&
    sleeppid=$!
    trap 'kill $sleeppid' TERM
    wait
fi

exit 0
