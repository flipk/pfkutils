#!/bin/sh

last_modified="2017-12-27.20:51:53"

. ./genkey.sh

root_ca_password=`random_text 40`
genkey Root-CA PFK-Org PFK-CA \
       'PFK CA Administration' ca@pfk.org $root_ca_password ''

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
