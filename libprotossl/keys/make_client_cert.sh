#!/bin/sh

last_modified="2018-01-13.01:07:35"

whoami="$1"
client_pass="$2"

if [ "x$whoami" = "x" ] ; then
    echo usage: please provide whoami
    exit 1
fi

. ./genkey.sh

if [ "x$client_pass" = "x" ] ; then
    client_pass=`random_text 40`
fi

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
