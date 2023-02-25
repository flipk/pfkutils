#!/bin/bash

set -e -x

# install chrome directly from google.
# DISABLED: chrome 110 does not pass signature validation!
#google=https://dl.google.com
#chrome_stable=$google/linux/direct/google-chrome-stable_current_x86_64.rpm
#dnf install -y $chrome_stable

dnf update -y
dnf install -y `cat /tmp/_setup_package_list_38.txt`

# install rpmfusion repos and keys.

fusion=https://download1.rpmfusion.org
fusionfree=$fusion/free/fedora/rpmfusion-free-release-38.noarch.rpm
fusionnonfree=$fusion/nonfree/fedora/rpmfusion-nonfree-release-38.noarch.rpm

dnf install -y $fusionfree $fusionnonfree

# now install a bunch of rpmfusion packages.

dnf install -y --allowerasing \
    vlc ffmpeg gimp-heif-plugin libheif

# see https://pypi.org/

pip -q install pip --upgrade
pip install \
     mypy-protobuf types-protobuf \
     jupyter numpy docx python-docx regex umap \
     pandas scipy scikit-learn matplotlib \
     statemachine python-statemachine \
     mypy-protobuf types-protobuf importlib-resources

cd /tmp
mv _setup_user_shell.sh /
chmod 755 /_setup_user_shell.sh

g++ _setup_su_reaper.cc -o /su_reaper

rm -f _setup*

exit 0
