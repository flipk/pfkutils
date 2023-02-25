#!/bin/bash

# TODO add -v's
mappings=""

# TODO support a custom command argument
cmd="/bin/bash"

# this is only really needed on fedora 33 docker 19.
# why?  "clone3" --> not supported, oops
seccomp_stupid="--security-opt seccomp=unconfined"

tag=pfk-fedora-38:1

USER=$( id -un )
container=${USER}-fedora38

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
