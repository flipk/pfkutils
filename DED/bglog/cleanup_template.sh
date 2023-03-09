#!/bin/sh

kill %d

done=0
while [ $done = 0 ] ; do
    result=$( ps ax | awk '{ if ($1 == %d) print "yes" }' )
    if [ x$result = xyes ] ; then
	echo still waiting
	sleep 0.5
    else
	done=1
    fi
done

echo process is dead
exit 0
