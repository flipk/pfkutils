#!/bin/bash

tag=blade-sshd:1
container=blade-sshd

exec docker run --rm -it -p 22:22 \
     --name $container --tmpfs=/tmp \
     $tag /start.sh
