#!/bin/bash

tag=pycharm-fedora41:1
tarfile=pycharm-fedora41-1.tar

# for some reason, Dockerfile COPY can't do ".." !!!
cp ../su_reaper.cc .

docker build -t $tag .

rm -f su_reaper.cc

echo to save:
echo docker save -o $tarfile $tag

exit 0
