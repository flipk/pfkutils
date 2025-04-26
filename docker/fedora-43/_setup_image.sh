#!/bin/bash

set -e -x

dnf update -y
dnf install -y `cat /tmp/_setup_package_list_43.txt`

# /usr/sbin/emacs points to /etc/alternatives/emacs
# which FOR SOME STUPID REASON insists on executing
# the broken emacs-30.1-pgtk.  make it do the right thing.
rm -f /usr/sbin/emacs
cd /usr/bin
rm -f emacs-30.1-pgtk
ln -s emacs-lucid emacs
cd /tmp

# install rpmfusion repos and keys.

fusion=https://download1.rpmfusion.org
fusionfree=$fusion/free/fedora/rpmfusion-free-release-43.noarch.rpm
fusionnonfree=$fusion/nonfree/fedora/rpmfusion-nonfree-release-43.noarch.rpm

dnf install -y $fusionfree $fusionnonfree

# now install a bunch of rpmfusion packages.

dnf install -y --allowerasing \
    vlc ffmpeg libheif libheif-tools

# install chrome directly from google.
google=https://dl.google.com
chrome_stable=$google/linux/direct/google-chrome-stable_current_x86_64.rpm
dnf install -y $chrome_stable

cd /tmp
mv _setup_user_shell.sh /
chmod 755 /_setup_user_shell.sh

g++ _setup_su_reaper.cc -o /su_reaper

rm -f _setup*

exit 0
