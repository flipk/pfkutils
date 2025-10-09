#!/bin/bash

tag=pfk-fedora-44:1
shortname=fedora44
tarfile=docker-pfk-fedora-44-1.tar

cp ../su_reaper.cc _setup_su_reaper.cc
cp ../template_setup_user_shell.sh _setup_user_shell.sh

sed \
    -e s/@@TAG@@/$tag/ \
    -e s/@@SHORTNAME@@/$shortname/ \
    < ../template_start.sh  \
    > start-fedora-44.sh

sed \
    -e s/@@SHORTNAME@@/$shortname/ \
    < ../template_join.sh  \
    > join-fedora-44.sh

chmod 755 start-fedora-44.sh join-fedora-44.sh

docker build --progress=plain -t $tag .

rm -f _setup_su_reaper.cc _setup_user_shell.sh

echo to save:
echo docker save -o $tarfile $tag

exit 0
