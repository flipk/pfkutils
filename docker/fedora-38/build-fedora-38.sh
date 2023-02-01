#!/bin/bash

tag=pfk-fedora-38:1
tarfile=docker-pfk-fedora-38-1.tar

docker build -t $tag .
docker save -o $tarfile $tag

exit 0
