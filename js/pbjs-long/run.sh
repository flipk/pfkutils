#!/bin/bash

pbjs=./node_modules/.bin/pbjs

set -e -x

$pbjs -o mypackage.js --force-long --target static-module -w commonjs --keep-case mypackage.proto

ls -l mypackage.proto mypackage.js

./test.js
./test2.js

rm -f mypackage.js
