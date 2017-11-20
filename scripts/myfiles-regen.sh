#!/bin/bash

dirlist="

< list dirs here >

"

egrep="list things|to remove|as a regex|here"

IFS=$'\n'
for d in $dirlist ; do
    echo doing $d 1>&2
    find "$d" -type f 2> /dev/null | egrep -v "$egrep" 
done > file_list.txt

exit 0
