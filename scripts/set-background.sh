#!/bin/sh


bg=$HOME/.background.png
found=0
# one of the following will succeed and the others
# will make an error msg. redirect the errors to null
if [ -f $bg ] ; then
    exec >/dev/null 2>/dev/null
    # if feh is installed, use that.
    feh --bg-tile $bg && found=1
    # if xsri is installed, use that.
    [ $found == 0 ] && xsri --tile=$bg && found=1
    # maybe 'display' is installed? gross.
    rootwin=`xdpyinfo | awk '/root window id/ { print $4 }'`
    [ $found == 0 ] && display -window $rootwin $bg && found=1
fi

# default bg if no image file or no program can be found.
[ $found = 0 ] && xsetroot -solid \#102030

if [ x$1 = xwait ] ; then
    sleep 3600&
    sleeppid=$!
    trap 'kill $sleeppid' TERM
    wait
fi

exit 0
