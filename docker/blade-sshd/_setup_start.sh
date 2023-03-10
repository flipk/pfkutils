#!/bin/bash

cd /tmp
/usr/sbin/sshd -E /var/log/sshd.log
tail -f /var/log/sshd.log

exit 0
