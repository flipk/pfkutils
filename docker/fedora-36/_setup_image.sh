#!/bin/bash

set -e -x

dnf install -y `cat /tmp/_setup_package_list_36.txt`

# install chrome directly from google.

google=https://dl.google.com
chrome_stable=$google/linux/direct/google-chrome-stable_current_x86_64.rpm

dnf install -y $chrome_stable

# install rpmfusion repos and keys.

fusion=https://download1.rpmfusion.org
fusionfree=$fusion/free/fedora/rpmfusion-free-release-36.noarch.rpm
fusionnonfree=$fusion/nonfree/fedora/rpmfusion-nonfree-release-36.noarch.rpm

dnf install -y $fusionfree $fusionnonfree

# now install a bunch of rpmfusion packages.
# note, some 'free' versions of some codecs are installed by default,
# and you have to provide the --allowerasing option so this can
# replace them with nonfree versions.

dnf install -y --allowerasing \
    vlc ffmpeg dosemu lame unace unrar xv \
    gimp-heif-plugin libheif \
    faad2-libs gsview handbrake handbrake-gui \
    libdca libde265 live555 x264 \
    exfat-utils fuse-exfat dosemu

# see https://pypi.org/

pip3 -q install pip --upgrade
pip3 install \
     mypy-protobuf types-protobuf \
     jupyter numpy docx python-docx regex umap \
     pandas scipy scikit-learn matplotlib

cd /tmp
mv _setup_user_shell.sh /
chmod 755 /_setup_user_shell.sh
rm -f setup*

exit 0
