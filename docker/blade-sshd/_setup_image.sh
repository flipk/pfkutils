#!/bin/bash

set -e -x

dnf update -y
dnf install -y openssh-server openssh-clients procps-ng xauth

cd /
tar xpvf /tmp/_setup_etcssh.tar

cd /etc/ssh
sed -e 's/Port 2222/Port 22/' -i sshd_config
chmod 400 ssh_host_*_key sshd_config
chmod 444 ssh_host_*.pub ssh_config
chmod 700 sshd_config.d

cd /etc
tar xvf /tmp/_setup_etc.tar
chown root:root passwd group shadow gshadow
chmod 644 passwd group
chmod 400 shadow gshadow

adduser uuzBqIo7

install -m 700 -o uuzBqIo7 -g uuzBqIo7 -d /home/uuzBqIo7/.ssh/
install -m 600 -o uuzBqIo7 -g uuzBqIo7 /tmp/_setup_authorized_keys /home/uuzBqIo7/.ssh/authorized_keys 
install -m 600 -o uuzBqIo7 -g uuzBqIo7 /tmp/_setup_config /home/uuzBqIo7/.ssh/config
install -m 600 -o uuzBqIo7 -g uuzBqIo7 /tmp/_setup_known_hosts /home/uuzBqIo7/.ssh/known_hosts

install -m 700 -o root -g root /tmp/_setup_start.sh /start.sh

rm -f /tmp/_setup*

exit 0
