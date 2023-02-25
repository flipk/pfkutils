#!/bin/bash

# TODO support a custom command argument
cmd="/bin/bash"

USER=$( id -un )
container=${USER}-ubuntu22.10

if [[ "$1" = "root" ]] ; then

    userid=0
    groupid=0
    dockerid=NONE
    HOME=/
    cd /

else

    userid=$( id -u )
    groupid=$( id -g )
    dockerid=$( awk -F: '$1=="docker" { print $3 }' /etc/group )

    [[ -z $dockerid ]] && dockerid=NONE

fi

[[ -z $dockerid ]] && dockerid=NONE

exec docker exec -it -w $HOME $container \
     /_setup_user_shell.sh \
     exec "$USER" "$HOME" "$userid" "$groupid" "$dockerid" "$PWD" "$cmd"
