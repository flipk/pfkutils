#!/bin/bash

tag=pfk-ubuntu-18.04:1
tarfile=docker-pfk-ubuntu-18.04-1.tar

ln -s ../su_reaper.cc
docker build -t $tag .
docker save -o $tarfile $tag
rm -f su_reaper.cc

exit 0
