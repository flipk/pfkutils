#!/bin/sh

# default bg if no image file can be found.
xsetroot -solid \#102030

bg="none"
if [ -f $HOME/tmp/grid3.png ] ; then
    bg=$HOME/tmp/grid3.png
elif [ -f $HOME/Dropbox/Public/pics/grid3.png ] ; then
    bg=$HOME/Dropbox/Public/pics/grid3.png
# more cases for other platforms go here
fi

# one of the following will succeed and the others
# will make an error msg. redirect the errors to null

if [ $bg != "none" ] ; then
    exec >/dev/null 2>/dev/null
    xsri --tile=$bg
    feh --bg-tile $bg
fi

exit 0
