#!/bin/bash

usage() {
    echo usage : make_git_links.sh dir_where_bit_bins_live
    exit 1
}

if [[ $# -ne 1 ]] ; then
    usage
fi

git_dir="$1"
cd "$HOME/pfk/$PFKARCH/bin"

git_progs="git git-lfs git-receive-pack git-shell git-upload-pack git-upload-archive gitk git-gui"

if [[ -x "${git_dir}/git" ]] ; then
    rm -f git_release g
    ln -s $git_dir git_release
    ln -s git g
    echo -n found:
    didnotfind=''
    for g in $git_progs ; do
        rm -f $g
        if [[ -f "${git_dir}/${g}" ]] ; then
            echo -n " ${g}"
            ln -s git_release/${g}
        else
            didnotfind="$didnotfind $g"
        fi
    done
    echo
    echo did not find: $didnotfind
else
    echo ERROR: no git there
    usage
fi

cd ~/pfk/

exit 0
