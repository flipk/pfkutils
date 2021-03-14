#!/bin/bash

awk '

BEGIN {
   printf( \
"         ----------video------------ -----audio-----\n" \
"duration cdec horz vert  kbps   fps  cdec   Hz  kbps    size(K) filename\n" \
"-------- ---- ---- ---- ----- ------ ---- ----- ---- ---------- --------\n" \
         );
   exit
}'

for f in $* ; do

    if [[ -f "$f" ]] ; then

        set -- $( ls -1s "$f"  )

        out=$( mktemp )
        ffprobe $f  > $out  2>&1

        if [[ $? = 0 ]] ; then

            FILENAME="$f" FILESIZE="$1" awk '

BEGIN {
   acodec = "NONE";
}

$1=="Duration:" {
   gsub(/\.[0-9][0-9],/, "", $2);
   duration=$2;
}

/Stream .* Video:/ {
   vcodec      = gensub(/^.*Video: ([a-zA-Z0-9]+).*$/, "\\1", "g", $0);
   dimensionsX = gensub(/^.* ([0-9]*)x([0-9]*).*$/, "\\1", "g", $0);
   dimensionsY = gensub(/^.* ([0-9]*)x([0-9]*).*$/, "\\2", "g", $0);
   vkbps       = gensub(/^.* ([0-9]*) kb.s.*$/, "\\1", "g", $0);
   fps         = gensub(/^.* ([0-9\.]*) fps.*$/, "\\1", "g", $0);
}

/Stream .* Audio:/ {
   acodec     = gensub(/^.*Audio: ([a-zA-Z0-9]+).*$/, "\\1", "g", $0);
   samplerate = gensub(/^.* ([0-9]*) Hz.*$/, "\\1", "g", $0);
   akbps      = gensub(/^.* ([0-9]*) kb.s.*$/, "\\1", "g", $0);
}

END {
   printf("%s %4s %4d %4d %5d %6.2f %4s %5u %4u %10u %s\n",
          duration, vcodec, dimensionsX, dimensionsY, vkbps, fps,
          acodec, samplerate, akbps,
          ENVIRON["FILESIZE"], ENVIRON["FILENAME"] );
}



' $out

            rm -f $out

        fi

    fi

done
