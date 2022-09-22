#!/bin/bash

USER=$( id -un )
container=${USER}-ubuntu18.04

exec docker exec -it \
     -u $(id -u):$(id -g) \
     -w $HOME \
     $container /bin/bash
