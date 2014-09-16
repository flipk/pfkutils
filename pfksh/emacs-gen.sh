#!/bin/sh

input=emacs.c
output=emacs_autogen.h

exec > $output

cat << E_O_F
/*
 * NOTE: THIS FILE WAS GENERATED AUTOMATICALLY FROM $input
 * BY emacs-gen.sh
 *
 * DO NOT BOTHER EDITING THIS FILE
 */
E_O_F

# Pass 1: print out lines before @START-FUNC-TAB@
#	  and generate defines and function declarations,
sed -e '1,/@START-FUNC-TAB@/d' -e '/@END-FUNC-TAB@/,$d' < $input |
  awk 'BEGIN { nfunc = 0; }
    /^[	 ]*#/ {
	print $0;
	next;
    }
    {
	fname = $2;
	c = substr(fname, length(fname), 1);
	if (c == ",")
	    fname = substr(fname, 1, length(fname) - 1);
	if (fname != "0") {
	    printf "#define XFUNC_%s %d\n", substr(fname, 3, length(fname) - 2), nfunc;
	    printf "static int %s ARGS((int c));\n", fname;
	    nfunc++;
	}
    }' || exit 1

exit 0
