#!/bin/bash

case "$1" in

    name)
        img=
        volname=
        mountpoint=
        ;;

    *)
        fail=1
        ;;

esac

set -e

if [[ $fail = 0 ]] ; then

    case "$2" in

        mount)
            if [[ $mountpoint != NONE ]] ; then
                if [[ -d $mountpoint ]] ; then
                    echo mountpoint $mountpoint already 'exists!'
                    exit 1
                fi
            fi
            sudo cryptsetup open --type luks $img $volname
            if [[ $mountpoint != NONE ]] ; then
                sudo mkdir -p $mountpoint
                sudo mount /dev/mapper/$volname $mountpoint
            fi
            fail=0
            ;;

        umount)
            if [[ $mountpoint != NONE ]] ; then
                sudo umount $mountpoint
                sudo rmdir $mountpoint
            fi
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
