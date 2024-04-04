#!/bin/bash

USER=$( id -un )

# NOTE AWS usernames can have '@' in them (gross)
SHORTUSER=${USER%@*}

container=${SHORTUSER}-pycharm-fedora41

if [[ "$1" = "root" ]] ; then
    exec docker exec -it $container /bin/bash
else
    exec docker exec -it \
         $container /_setup_user_shell.sh exec "$PWD" "$SHORTUSER"
fi
