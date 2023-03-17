#!/bin/bash

tag=blade-sshd:1
container=blade-sshd

mkdir -p $TMP/sshd-logs
touch    $TMP/sshd-logs/sshd.log

docker run --rm -p 22:22 -d \
     --name $container --tmpfs=/tmp \
     -v $TMP/sshd-logs:/logs \
     $tag /start.sh

echo docker $container started

exit 0
