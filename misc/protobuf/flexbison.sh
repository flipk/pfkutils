#!/bin/bash

set -e -x

bison -d protobuf_parser.yy -o protobuf_parser.cc
flex protobuf_tokenizer.ll

exit 0
