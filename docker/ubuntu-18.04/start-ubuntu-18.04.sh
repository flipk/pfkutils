#!/bin/bash

tag=pfk-ubuntu-18.04:1
container=${USER}-ubuntu18.04

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
     -v $HOME:$HOME \
     -v /tmp/.X11-unix:/tmp/.X11-unix \
     $tag /_setup_user_shell.sh "$entry"
