#!/bin/bash

enckey="-e password_here"
variant="-v 5"
#compr="-c none"
#debug="-d 127"
maxsize="-m 15000"

rm -rf test
mkdir test

set -e -x

cp Makefile test/0
cp obj/uuz test/1
tar cf test/2 obj
cd test

../obj/uuz e $debug $variant $compr -t $enckey -o 3 $maxsize 0 1 2
mv 0 00
mv 1 11
mv 2 22
# assumption: when bash does "3.*" globbing, the results
# are listed in numeric (actually alphabetical) order.
# this wouldn't work if they were in random order.
cat 3.* | ../obj/uuz d $debug $enckey -

sha256sum 0 00 1 11 2 22
ls -l 3.???
cmp 0 00
cmp 1 11
cmp 2 22

exit 0
