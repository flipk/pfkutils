#!/bin/bash

tag=pfk-fedora-38:1
container=${USER}-fedora38

USER=$( id -un )
entry=`grep $USER /etc/passwd`
if [[ $? -ne 0 ]] ; then
    entry=`adquery user $USER`
    if [[ $? -ne 0 ]] ; then
        echo "can't figure out passwd entry"
        exit 1
    fi
fi

exec docker run --rm -it \
     --name $container \
     -v $HOME:$HOME --tmpfs=/tmp \
     --hostname=$(hostname) --env DISPLAY=$DISPLAY \
     -v /tmp/.X11-unix:/tmp/.X11-unix \
     $tag /_setup_user_shell.sh "$entry"
