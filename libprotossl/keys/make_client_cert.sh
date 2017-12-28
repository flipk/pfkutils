#!/bin/sh

last_modified="2017-12-27.21:08:47"

whoami=$1

if [ "x$whoami" == "x" ] ; then
    echo usage: please provide whoami
    exit 1
fi

. ./genkey.sh

client_pass=`random_text 40`

genkey my-certificate PFK-Org PFK-Client \
       "$whoami" client@pfk.org $client_pass Root-CA

exit 0

# Local Variables:
# mode: shell-script
# indent-tabs-mode: nil
# tab-width: 8
# eval: (add-hook 'write-file-hooks 'time-stamp)
# time-stamp-start: "last_modified="
# time-stamp-format: "\"%:y-%02m-%02d.%02H:%02M:%02S\""
# time-stamp-end: "$"
# End:
