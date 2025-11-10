#!/bin/bash

protoc --python_out=. --mypy_out=. websocket_test.proto

exit 0
