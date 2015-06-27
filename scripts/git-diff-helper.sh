#!/bin/sh

width=`stty size | awk '{print $2}'`
diff -btyW $width $2 $5 | fmtsdiff $width | less '+/%[<>|]%'

exit 0
