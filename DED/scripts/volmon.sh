#!/bin/bash

volmon=$0

#if [[ "x$1" == "x" ]] ; then
#    echo starting urxvt
#    urxvt -title VOLMON -e $volmon internal
#    echo urxvt is done
#    exit 0
#fi

if [[ "x$TMP" == "x" ]] ; then
    TMP=/tmp
fi

rm -rf $TMP/volmon
mkdir -p $TMP/volmon
a=$TMP/volmon/a
b=$TMP/volmon/b

while [[ "x$answer" != "xq" ]]  ; do
    lsblk -o NAME,FSTYPE,LABEL,MOUNTPOINT > $b
    if cmp -s $a $b ; then
	if read -t 1 answer ; then
	    rm $a
	fi
    else
	clear
	cat $b
	mv $b $a
    fi
done

exit 0
