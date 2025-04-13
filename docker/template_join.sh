#!/bin/bash

shortname=@@SHORTNAME@@

USER=$( id -un )
container=${USER}-${shortname}

if [[ "$1" = "root" ]] ; then
    userid=0
    groupid=0
    HOME=/
    USER=root
    cd /
    shift
else
    userid=$( id -u )
    groupid=$( id -g )
fi

if [[ $# -eq 0 ]] ; then
    cmd=("/bin/bash")
else
    cmd=()
    while [[ $# -gt 0 ]] ; do
        # TODO check if this is a bash5-only feature.
        cmd+=("$1")
        shift
    done
fi

dockerid=$( awk -F: '$1=="docker" { print $3 }' /etc/group )
[[ -z $dockerid ]] && dockerid=NONE

exec docker exec -it -w $HOME $container \
     /_setup_user_shell.sh \
     exec "$USER" "$HOME" "$userid" "$groupid" "$dockerid" "$PWD" \
     "${cmd[@]}"
