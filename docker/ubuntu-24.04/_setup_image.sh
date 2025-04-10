#!/bin/bash

set -x

cd /tmp

apt-get update -y

# this prevents an interactive prompt during install.
ln -fs /usr/share/zoneinfo/America/Chicago /etc/localtime
DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata

# this prevents wireshark from asking about permissions.
echo "wireshark-common wireshark-common/install-setuid boolean false" | debconf-set-selections

apt-get install -y $( cat _setup_package_list_24.04.txt )

apt-get install -y \
        mypy-protobuf python3-websockets python3-numpy \
        python3-regex python3-pandas python3-scipy \
        python3-matplotlib python3-importlib-resources

cd /tmp
mv _setup_user_shell.sh /
chmod 755 /_setup_user_shell.sh

g++ _setup_su_reaper.cc -o /su_reaper

rm -f _setup*

exit 0
