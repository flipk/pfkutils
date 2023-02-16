#!/bin/bash

if [[ ! -f "$1" ]] ; then
    echo 'usage:  ldd-findpaths  <executable>'
    echo '    will display full paths to all'
    echo '    .so files used by this executable'
    exit 1
fi

ldd $1 | sed -e 's/^.* => \(.*\) .0x.*$/\1/' | grep ^/
