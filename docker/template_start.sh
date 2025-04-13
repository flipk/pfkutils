#!/bin/bash

custom_mappings=""
tag=@@TAG@@
shortname=@@SHORTNAME@@

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

# for ind in $( seq 0  $(( ${#cmd[@]} - 1 )) ) ; do
#     echo cmd $ind = "${cmd[$ind]}"
# done

mappings="-v $HOME:$HOME -v /tmp/.X11-unix:/tmp/.X11-unix"
envs="--env DISPLAY=$DISPLAY"
USER=$( id -un )
container=${USER}-${shortname}
userid=$( id -u )
groupid=$( id -g )
dockerid=$( awk -F: '$1=="docker" { print $3 }' /etc/group )
[[ -z $dockerid ]] && dockerid=NONE

exec docker run --rm -it --net host --hostname=$(hostname) \
     --name $container --tmpfs=/tmp $mappings $custom_mappings $envs \
     $tag /_setup_user_shell.sh \
     start "$USER" "$HOME" "$userid" "$groupid" "$dockerid" "$PWD" \
     "${cmd[@]}"
