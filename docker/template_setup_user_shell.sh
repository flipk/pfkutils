#!/bin/bash

start_or_exec="$1"
shift
export USER="$1"
shift
export HOME="$1"
shift
userid="$1"
shift
groupid="$1"
shift
dockerid="$1"
shift
current_dir="$1"
shift

cmd=()
while [[ $# -gt 0 ]] ; do
    # TODO check if this is a bash5-only feature.
    cmd+=("$1")
    shift
done

# for ind in $( seq 0  $(( ${#cmd[@]} - 1 )) ) ; do
#     echo cmd $ind = "${cmd[$ind]}"
# done

if [[ "$start_or_exec" = "start" ]] ; then

    (
        groupadd -g $groupid "$USER" || true
        useradd -m -u $userid -g $groupid -o -s /bin/bash "$USER" -d "$HOME"
        usermod -a -G "$USER" "$USER"
        if [[ ! $dockerid = "NONE" ]] ; then
            groupadd -g $dockerid docker || true
            groupmod -g $dockerid docker || true
            usermod -a -G docker "$USER"
        fi
    ) > /dev/null 2>&1

elif [[ "$start_or_exec" = "exec" ]] ; then

    echo 'joining container already in progress'

else

    echo ERROR: first arg must be start or exec
    exit 1

fi

cd "$current_dir"
exec /su_reaper $userid $groupid $dockerid "${cmd[@]}"
