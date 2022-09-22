#!/bin/bash

tag=pfk-ubuntu-18.04:1
tarfile=docker-pfk-ubuntu-18.04-1.tar

docker build -t $tag .
docker save -o $tarfile $tag

exit 0
