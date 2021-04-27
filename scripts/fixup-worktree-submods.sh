#!/bin/bash

# gitdir points to the original .git
# commondir points to the dir containing the usual things
#  like branches, info, lfs, refs, packed-refs
# but we make our own HEAD, logs/ and index.
# a .git file can contain "gitdir: ....path-to-repo"

# $1 : /full/path/to/worktrees/supertopgitdir
# $2 : /full/path/to/worktrees/supertopgitdir/modules/gitdir

check_one_repo() {

    local supergitdir="$1"
    local gitdir="$2"

    relgitdir="${gitdir#$supergitdir/}"
    origgitdir="${supergitdir%/.git/worktrees*}/.git/${relgitdir}"

    echo checking $gitdir

    good=1
    if [[ "x$gitdir" == "x$relgitdir" ]] ; then
        echo ERROR gitdir not a subdir of supergitdir
        good=0
    elif [[ ! -d "$supergitdir" ]] ; then
        echo ERROR no $supergitdir
        good=0
    elif [[ ! -d "$gitdir" ]] ; then
        echo ERROR no $gitdir
        good=0
    elif [[ ! -f "$supergitdir/commondir" ]] ; then
        echo ERROR no $supergitdir/commondir
        good=0
    elif [[ ! -f "$supergitdir/gitdir" ]] ; then
        echo ERROR no $supergitdir/gitdir
        good=0
    elif [[ -f "$gitdir/commondir" ]] ; then
        echo ERROR already have $gitdir/commondir
        good=0
    elif [[ -f "$gitdir/gitdir" ]] ; then
        echo ERROR already have $gitdir/gitdir
        good=0
    fi
    if [[ $good = 0 ]] ; then
        echo usage: fixup_one_repo supergitdir gitdir
        exit 1
    fi

}

fixup_one_repo() {

    local supergitdir="$1"
    local gitdir="$2"

    relgitdir="${gitdir#$supergitdir/}"
    origgitdir="${supergitdir%/.git/worktrees*}/.git/${relgitdir}"

    echo "doing echo gitdir=$gitdir"
    echo "      supergitdir=$supergitdir"
    echo "      relgitdir=$relgitdir"
    echo "      origgitdir=$origgitdir"

    cd $gitdir
    if [[ -f $origgitdir/HEAD ]] ; then
        rm -rf branches config lfs info objects packed-refs refs
        cp $supergitdir/gitdir .
        echo "${origgitdir}" > commondir
    else
        echo NOTE: skipping because no orig HEAD
    fi

}

gr=$(git-root)

if [[ ! -f "$gr/.git" ]] ; then
    echo 'ERROR not in a worktree?'
    exit 1
fi

cd "$gr"
supergitdir=$( awk '/gitdir:/ { print $2}' .git )
echo supergitdir=$supergitdir
gits="$( find . -type f -name .git | grep -v \\./\\.git )"
numgitdirs=0
for f in $gits ; do
    d=${f%/.git}
    d=$gr/${d#./}
    cd $d
    gitdir=$( awk '/gitdir:/ { print $2}' .git )
    cd $gitdir
    gitdir=$PWD
    gitdirs[$numgitdirs]=$gitdir
    (( numgitdirs++ ))
done

# first sanity check everything, touch nothing.
# checker will exit if anything's wrong.

count=0
while [[ $count -lt $numgitdirs ]] ; do
    gitdir=${gitdirs[$count]}
    check_one_repo $supergitdir $gitdir
    (( count++ ))
done

# if we live this long, do the thing.

count=0
while [[ $count -lt $numgitdirs ]] ; do
    gitdir=${gitdirs[$count]}
    fixup_one_repo $supergitdir $gitdir
    (( count++ ))
done

exit 0
