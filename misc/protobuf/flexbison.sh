#!/bin/bash

set -e -x

bison -d protobuf_json_parser.yy -o protobuf_json_parser.cc
flex protobuf_json_tokenizer.ll

exit 0
