#!/bin/bash

do_one_file() {
    input="$1"
    suffix="${input##*.}"
    if [[ "x${suffix}" != "xmp4" ]] ; then
	echo input : $input
	echo this program only works on mp4 videos
	exit 1
    fi
    output="${input%.*}.jpg"
    regen=0
    if [[ ! -f "$output" ]] ; then
	echo no thumb: $input
	regen=1
    elif [[ "$input" -nt "$output" ]] ; then
	echo old thumb: $input
	regen=1
    fi
    if [[ $regen == 1 ]] ; then
	duration=`ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$input"`
	if [[ $? -ne 0 ]] ; then
	    echo ffprobe failed
	    exit 1
	fi
	half_duration=`dc -e "6k $duration 2 / f"`
	echo duration = $duration, half = $half_duration
	ffmpeg -y -i "$input" -ss $half_duration -vframes 1 -q:v 2 -s 256x160 "$output" > /dev/null 2>&1
	ls -l "$output"
    fi
}

while [[ $# -gt 0 ]] ; do
    do_one_file  "$1"
    shift
done

exit 0
