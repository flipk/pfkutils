#!/bin/bash

cd /
exec /usr/sbin/sshd -D -E /logs/sshd.log
