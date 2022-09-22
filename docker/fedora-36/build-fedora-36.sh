#!/bin/bash

tag=pfk-fedora-36:1
tarfile=docker-pfk-fedora-36-1.tar

docker build -t $tag .
docker save -o $tarfile $tag

exit 0
