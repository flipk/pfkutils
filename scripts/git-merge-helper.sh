#!/bin/sh

# xxx the following is subject to command line substitution
# git config --global mergetool.pfkmerge.cmd $PFKUTILS_PREFIX/bin/git-merge-helper.sh "$LOCAL" "$REMOTE" "$BASE" "$MERGED"
# git config --global mergetool.pfkmerge.trustExitCode false
# git config --global merge.tool pfkmerge
# git mergetool <File>

$PFKUTILS_PREFIX/bin/merge3 "$1" "$2" "$3" "$4"

exit 0
