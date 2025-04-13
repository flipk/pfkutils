#!/bin/bash

# TODO add -v's
mappings=""

# TODO support a custom command argument
cmd="/bin/bash"

tag=pfk-ubuntu-22.04:1

USER=$( id -un )
container=${USER}-ubuntu22.04

userid=$( id -u )
groupid=$( id -g )
dockerid=$( awk -F: '$1=="docker" { print $3 }' /etc/group )

[[ -z $dockerid ]] && dockerid=NONE

exec docker run --rm -it \
     --name $container --tmpfs=/tmp \
     -v $HOME:$HOME $mappings \
     -v /tmp/.X11-unix:/tmp/.X11-unix \
     --hostname=$(hostname) --env DISPLAY=$DISPLAY \
     $tag /_setup_user_shell.sh \
     start "$USER" "$HOME" "$userid" "$groupid" "$dockerid" "$PWD" "$cmd"
