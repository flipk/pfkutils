#!/bin/bash

set -e -x

bison -d json_parser.yy -o json_parser.cc
flex json_tokenizer.ll

exit 0
