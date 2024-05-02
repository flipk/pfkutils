#!/bin/bash

set -e -x

dir=${0%/rebuild-proto.sh}
cd "$dir"

protoc --python_out=. --mypy_out=. MyFS.proto

ls -l MyFS.proto MyFS_pb2.py MyFS_pb2.pyi

exit 0
