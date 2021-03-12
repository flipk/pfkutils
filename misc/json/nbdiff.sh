#!/bin/bash

nbtopy=/home/pknaack1/proj/nbtopy/obj/nbtopy

first="$1"
second="$2"

if [[ ! -f "$first" ]] ; then
    echo ERROR: cant find $first
    exit 1
fi

if [[ ! -f "$second" ]] ; then
    echo ERROR: cant find $second
    exit 1
fi

tfile1=$( mktemp )
tfile2=$( mktemp )

$nbtopy $first > $tfile1
$nbtopy $second > $tfile2

diff $tfile1 $tfile2
rm -f $tfile1 $tfile2

exit 0
