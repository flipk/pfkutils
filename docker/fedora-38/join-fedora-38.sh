#!/bin/bash

USER=$( id -un )
container=${USER}-fedora38

exec docker exec -it \
     -u $(id -u):$(id -g) \
     -w $HOME \
     $container /bin/bash
