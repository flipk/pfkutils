#!/bin/bash

set -x

cd /tmp

apt-get update -y

# this prevents an interactive prompt during install.
ln -fs /usr/share/zoneinfo/America/Chicago /etc/localtime
DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata

# this prevents wireshark from asking about permissions.
echo "wireshark-common wireshark-common/install-setuid boolean false" | debconf-set-selections

apt-get install -y $( cat _setup_package_list_18.04.txt )

pip3 -q install pip --upgrade
pip3 install \
     mypy-protobuf types-protobuf \
     jupyter numpy docx python-docx regex umap \
     pandas scipy scikit-learn matplotlib

exit 0
