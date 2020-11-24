#!/bin/bash

fs_list=(fs1 fs2 fs3)
#               img                 volname    mountpoint
fs_params_fs1=( /path/to/file.img   luks-fs1   /mount/fs1 )
fs_params_fs2=( UUID=xxxx           luks-fs3   NONE       )
fs_params_fs3=( UUID=xxxx           luks-fs3   NONE       )

specials=(script1 script2)

special_script1() {
    pfk-fs.sh m fs1
    pfk-fs.sh m fs2
}
special_script2() {
    pfk-fs.sh u fs1
    pfk-fs.sh u fs2
}

# above this line is customized on install

#echo fs_list =  ${fs_list[@]}
#ind=0
#while [[ $ind -lt ${#fs_list[@]} ]] ; do
#    fs=${fs_list[$ind]}
#    echo fs $ind = $fs
#    declare -n params=fs_params_${fs}
#    echo fs $fs img = ${params[0]}
#    echo fs $fs volname = ${params[1]}
#    echo fs $fs mountpoint = ${params[2]}
#    (( ind++ ))
#done

fail=1

if [[ "$1" = m ]] ; then
    fail=0
    mount=1
    op="mount"
elif [[ "$1" = u ]] ; then
    fail=0
    mount=0
    op="unmount"
else
    for special in ${specials[@]} ; do
        if [[ "$1" = $special ]] ; then
            special_$special
            # exit here
            exit 0
        fi
    done
fi

if [[ $fail = 0 ]] ; then
    fail=1
    for fscand in ${fs_list[@]} ; do
        if [[ "$2" = $fscand ]] ; then
            fs=$fscand
            # declare an 'alias' variable
            # since i can't find any other way to indirectly name
            # an array variable and reference its members.
            declare -n params=fs_params_${fs}
            img=${params[0]}
            volname=${params[1]}
            mountpoint=${params[2]}
            fail=0
        fi
    done
fi

if [[ $fail = 0 ]] ; then
    echo about to $op $fs
    if [[ $mount = 1 ]] ; then
        set -e
        sudo cryptsetup open --type luks $img $volname
        if [[ $mountpoint != NONE ]] ; then
            sudo mkdir -p $mountpoint
            sudo mount /dev/mapper/$volname $mountpoint
        fi
    else
        set -e
        if [[ $mountpoint != NONE ]] ; then
            sudo umount $mountpoint
            sudo rmdir $mountpoint
        fi
        sudo cryptsetup close $volname
    fi
fi

if [[ $fail = 1 ]] ; then

    echo usage : $0 '<op> <fs>'
    echo '   where <op> is one of : m u' ${specials[@]}
    echo '   and <fs> is one of :' ${fs_list[@]}

fi

exit 0
