#!/bin/bash

set -e -x

dnf install -y `cat /tmp/_setup_package_list_41.txt`
dnf update -y

pip install \
    protobuf mypy-protobuf types-protobuf python-prctl \
    python-protobuf numpy umap pandas scipy scikit-learn \
    matplotlib websockets

cd /tmp
mv _setup_user_shell.sh /
chmod 755 /_setup_user_shell.sh
rm -f _setup*

mkdir -p /opt
cd /opt
tar zxf /tmp/pycharm-community-2023.3.5.tar.gz
rm -f /tmp/pycharm-community-2023.3.5.tar.gz

cd /bin
ln -s /opt/pycharm-community-2023.3.5/bin/pycharm.sh

cd /tmp
g++ -O3 su_reaper.cc -o /su_reaper
rm -f su_reaper.cc

exit 0
