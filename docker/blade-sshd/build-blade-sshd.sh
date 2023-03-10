#!/bin/bash

if [[ ! -f start-blade-ssh.sh ]] ; then
    echo please run this in the blade-sshd docker build directory
    exit 1
fi

if [[ ! -f _setup_authorized_keys ]] ; then
    echo please place _setup_authorized_keys in this directory
    exit 1
fi

if [[ ! -f _setup_config ]] ; then
    echo please place _setup_config in this directory
    exit 1
fi

tag=blade-sshd:1

sudo tar cf /tmp/_setup_etcssh.tar /etc/ssh
cp /tmp/_setup_etcssh.tar .
chmod 600 _setup_etcssh.tar
sudo rm -f /tmp/_setup_etcssh.tar

docker build -t $tag .

rm -f _setup_etcssh.tar

exit 0
