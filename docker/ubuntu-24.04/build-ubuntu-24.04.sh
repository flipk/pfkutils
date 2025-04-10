#!/bin/bash

tag=pfk-ubuntu-24.04:1
tarfile=docker-pfk-ubuntu-24.04-1.tar

cp ../su_reaper.cc _setup_su_reaper.cc

docker build -t $tag .

rm -f _setup_su_reaper.cc

docker save -o $tarfile $tag

exit 0
