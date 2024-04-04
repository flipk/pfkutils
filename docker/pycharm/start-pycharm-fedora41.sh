#!/bin/bash

USER=$( id -un )
USERID=$( id -u )
GROUPID=$( id -g )

# NOTE AWS usernames can have '@' in them (gross)
SHORTUSER=${USER%@*}

tag=pycharm-fedora41:1
container=${SHORTUSER}-pycharm-fedora41

docker_group_id=-1

mappings=""

set -x

exec docker run --rm -it \
     --name $container --net host \
     -v $HOME:$HOME \
     -v /var/run/docker.sock:/var/run/docker.sock \
     $mappings \
     --hostname=$(hostname) --env DISPLAY=$DISPLAY \
     -v /tmp/.X11-unix:/tmp/.X11-unix \
     $tag /_setup_user_shell.sh start "$PWD" "$SHORTUSER" "$HOME" $USERID $GROUPID $docker_group_id
