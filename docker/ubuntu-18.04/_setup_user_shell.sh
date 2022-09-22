#!/bin/bash

entry="$1"
IFS=":"
set -- $entry
IFS=" "
username="$1"
userid="$3"
groupid="$4"
comment="$5"
homedir="$6"

(
    # ignore all group/user errors.
    groupadd -g $groupid ${userid}-group
    useradd -m -u $userid -g $groupid -o -s /bin/bash \
            $prefix$username -d $homedir --comment "$comment"
) > /dev/null 2>&1

exec su -s /bin/bash - $username
