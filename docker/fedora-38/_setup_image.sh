#!/bin/bash

set -e -x

# install rpmfusion repos and keys.

fusion=https://download1.rpmfusion.org
fusionfree=$fusion/free/fedora/rpmfusion-free-release-38.noarch.rpm
fusionnonfree=$fusion/nonfree/fedora/rpmfusion-nonfree-release-38.noarch.rpm

dnf install -y $fusionfree $fusionnonfree

# now install a bunch of rpmfusion packages.

dnf install -y \
    vlc ffmpeg dosemu lame unace unrar \
    gimp-heif-plugin libheif

# install chrome directly from google.

#google=https://dl.google.com
#chrome_stable=$google/linux/direct/google-chrome-stable_current_x86_64.rpm
#dnf install -y $chrome_stable
# chrome 109 doesn't pass signature !!!


dnf install -y `cat /tmp/_setup_package_list_38.txt`

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
rm -f setup*

exit 0
