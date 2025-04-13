#!/bin/bash

set -e -x

cd /tmp

apt-get update -y

# this prevents an interactive prompt during install.
ln -fs /usr/share/zoneinfo/America/Chicago /etc/localtime
DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata

# this prevents wireshark from asking about permissions.
echo "wireshark-common wireshark-common/install-setuid boolean false" | debconf-set-selections

apt-get install -y $( cat _setup_package_list_18.04.txt )

cd /tmp
mv _setup_user_shell.sh /
chmod 755 /_setup_user_shell.sh

g++ _setup_su_reaper.cc -o /su_reaper

rm -f _setup*

exit 0
