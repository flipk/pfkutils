#!/bin/sh

# xxx the following is subject to command line substitution
# git config --global mergetool.pfkmerge.cmd $HOME/pfk/bin/git-merge-helper.sh "$LOCAL" "$REMOTE" "$BASE" "$MERGED"
# git config --global mergetool.pfkmerge.trustExitCode false
# git config --global merge.tool pfkmerge
# git mergetool <File>

lisp=$HOME/merge3.$$

cat << EOF > $lisp
(setq pfk-small-font
      "-misc-*-*-r-semicondensed-*-13-120-*-*-*-*-iso8859-1")
(defun my-make-frame-hook (frame)
  ""
  (interactive)
  (progn
    (select-frame frame)
    (set-frame-font pfk-small-font)
    (set-foreground-color "yellow")
    (set-background-color "black")
    (set-cursor-color "red")
    ))
(add-hook 'after-make-frame-functions 'my-make-frame-hook)
(tool-bar-mode 0)
(ediff-merge-files-with-ancestor "$1" "$2" "$3" nil "$4")
EOF

emacs -fn '-misc-*-*-r-semicondensed-*-13-120-*-*-*-*-iso8859-1' \
    -g 162x65+25+54 --no-init-file --load $lisp

rm -f $lisp

exit 0
