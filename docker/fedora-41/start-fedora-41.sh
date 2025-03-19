#!/bin/bash

# TODO add -v's
mappings=""

# TODO support a custom command argument
cmd="/bin/bash"

seccomp_stupid="--security-opt seccomp=unconfined"

tag=pfk-fedora-41:1

USER=$( id -un )
container=${USER}-fedora41

userid=$( id -u )
groupid=$( id -g )
dockerid=$( awk -F: '$1=="docker" { print $3 }' /etc/group )

[[ -z $dockerid ]] && dockerid=NONE

exec docker run --rm -it \
     $seccomp_stupid \
     --name $container --tmpfs=/tmp \
     -v $HOME:$HOME $mappings \
     -v /tmp/.X11-unix:/tmp/.X11-unix \
     --hostname=$(hostname) --env DISPLAY=$DISPLAY \
     $tag /_setup_user_shell.sh \
     start "$USER" "$HOME" "$userid" "$groupid" "$dockerid" "$PWD" "$cmd"
