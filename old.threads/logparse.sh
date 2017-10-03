#!/bin/sh

cat log1 log2 | sort | awk '

BEGIN {
    first=1;
    lastt=0;
    firstpid=0;
}

{
   if ( first == 1 )
   {
       lastt = $1;
       first = 0;
       firstpid = $2;
   }
   diff = $1 - lastt;
   lastt = $1
   $1 = "";
   if ( $2 == firstpid )
      $2 = "aa  ";
   else
      $2 = "  bb";
   printf( "%6d %s\n", diff, $0 );
}

'  > log3
