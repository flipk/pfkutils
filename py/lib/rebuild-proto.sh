#!/bin/bash

protofile="$1"

set -e -x

protoc --python_out=. --mypy_out=. \
    --experimental_allow_proto3_optional \
    "${protofile}"

exit 0
