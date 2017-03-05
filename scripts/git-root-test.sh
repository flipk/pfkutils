#!/bin/bash

dirs="
/home/flipk/tmp
/home/flipk/tmp/pfkutils
/home/flipk/tmp/pfkutils/contrib/ctwm-3.8a
/home/flipk/tmp/pfkutils/.git
/home/flipk/tmp/pfkutils/.git/objects
/home/flipk/tmp/pfkutils/.git/modules/SUBMOD_1
/home/flipk/tmp/pfkutils/.git/modules/SUBMOD_1/objects
/home/flipk/tmp/pfkutils/.git/modules/SUBMOD_1/modules/submods/SUBMOD_2
/home/flipk/tmp/pfkutils/.git/modules/SUBMOD_1/modules/submods/SUBMOD_2/objects
/home/flipk/tmp/pfkutils/SUBMOD_1
/home/flipk/tmp/pfkutils/SUBMOD_1/contrib/ctwm-3.8a
/home/flipk/tmp/pfkutils/SUBMOD_1/submods/SUBMOD_2
/home/flipk/tmp/pfkutils/SUBMOD_1/submods/SUBMOD_2/contrib/ctwm-3.8a
/home/flipk/Dropbox/Public/git/pfkutils.git/
/home/flipk/Dropbox/Public/git/pfkutils.git/objects
"

for d in $dirs ; do
    cd $d
    echo ''
    echo ' *********** ' $PWD ' *********** '
    git-root -v -d
done
echo ''

exit 0
