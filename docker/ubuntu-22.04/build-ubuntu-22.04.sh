#!/bin/bash

tag=pfk-ubuntu-22.04:2
shortname=ub22
tarfile=docker-pfk-ubuntu-22.04-2.tar

cp ../su_reaper.cc _setup_su_reaper.cc
cp ../template_setup_user_shell.sh _setup_user_shell.sh

sed \
    -e s/@@TAG@@/$tag/ \
    -e s/@@SHORTNAME@@/$shortname/ \
    < ../template_start.sh  \
    > start-ubuntu-22.sh

sed \
    -e s/@@SHORTNAME@@/$shortname/ \
    < ../template_join.sh  \
    > join-ubuntu-22.sh

chmod 755 start-ubuntu-22.sh join-ubuntu-22.sh

docker build -t $tag .

rm -f _setup_su_reaper.cc

echo to save:
echo docker save -o $tarfile $tag

exit 0
