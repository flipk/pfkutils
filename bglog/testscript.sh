#!/bin/sh

echo id : `id`
sleep 1
echo tty : `tty`
sleep 1
echo pid : $$
sleep 1

count=1
while [ $count -lt 200 ] ; do
    echo this is line $count of 200 of random data that takes up space
    count=$(( $count + 1 ))
    sleep 0.1
done

exit 0
