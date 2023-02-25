#!/bin/bash

tag=pfk-ubuntu-22.10:1
tarfile=docker-pfk-ubuntu-22.10-1.tar

cp ../su_reaper.cc _setup_su_reaper.cc

docker build -t $tag .

rm -f _setup_su_reaper.cc

docker save -o $tarfile $tag

exit 0
