#!/bin/bash

fail=1

if [[ "$1" = "name" ]] ; then

    img=
    volname=
    mountpoint=
    label=
    fail=0

elif [[ "$1" = "name" ]] ; then

    img=
    volname=
    mountpoint=
    label=
    fail=0

else

    fail=1

fi

set -e

if [[ $fail = 0 ]] ; then

    case "$2" in

        mount)
            if [[ -d $mountpoint ]] ; then
                echo mountpoint $mountpoint already 'exists!'
                exit 1
            fi
            sudo cryptsetup open --type luks $img $volname
            sudo mkdir -p $mountpoint
            sudo mount /dev/mapper/$volname $mountpoint
            fail=0
            ;;

        umount)
            sudo umount $mountpoint
            sudo rmdir $mountpoint
            sudo cryptsetup close $volname
            fail=0
            ;;

        *)
            fail=1
            ;;

    esac

fi

if [[ $fail = 1 ]] ; then

    echo usage: $0 '<alt | altbak> <mount | umount>'
    exit 1

fi

exit 0
