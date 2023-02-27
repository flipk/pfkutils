#!/bin/bash

dev=$( tty )
dev=${dev#/dev/}

pids=$( ps x | egrep -v "$dev|tickler|PID TTY" | awk '{print $1}' )

kill $pids

exit 0
