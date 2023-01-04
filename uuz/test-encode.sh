#!/bin/bash

enckey="-e password_here"
variant="-v 4"
#compr="-c none"
#debug="-d 127"
maxsize="-m 15000"

rm -rf test
mkdir test

set -e -x

file1=1-Makefile
file2=2-obj-uuz-binary
file3=3-big-tar-file.tar
files="$file1 $file2 $file3"

cp Makefile test/$file1
chmod 400 test/$file1
cp obj/uuz test/$file2
tar cf test/$file3 obj
chmod 777 test/$file3
cd test

../obj/uuz e $debug $variant $compr -t $enckey -o 4.uuz $maxsize $files
sha256sum $files > before.txt
# assumption: when bash does "3.*" globbing, the results
# are listed in numeric (actually alphabetical) order.
# this wouldn't work if they were in random order.
cat 4.uuz.* | ../obj/uuz d $debug $enckey -
sha256sum $files > after.txt
diff before.txt after.txt
cat 4.uuz.* | ../obj/uuz t $debug $enckey -


exit 0
