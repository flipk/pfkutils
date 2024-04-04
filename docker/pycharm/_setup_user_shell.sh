#!/bin/bash

docker_group_id=-1

if [[ x"$1" = xstart ]] ; then

    current_dir="$2"
    username="$3"
    homedir="$4"
    userid="$5"
    groupid="$6"

    (
        groupadd -g $groupid $username
        useradd -m -u $userid -g $groupid -o -s /bin/bash \
                $username -d $homedir
        usermod -a -G $username $username
    ) > /dev/null 2>&1

    mkdir -p /work
    chown $userid:$groupid /work
    set -- /bin/bash

elif [[ x"$1" = xexec ]] ; then

    current_dir="$2"
    username="$3"

    eval $( awk -F: '$1=="'$username'" { print "userid="$3" groupid="$4" homedir="$6 }' /etc/passwd )

    set -- /bin/bash

else

    echo ERROR: first arg must be start or exec
    exit 1

fi

export HOME="$homedir"
cd "$current_dir"

exec /su_reaper $userid $groupid $docker_group_id "$@"
