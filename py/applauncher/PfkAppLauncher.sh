#!/bin/bash

set -e -x

cd ${HOME}/pfk/PfkAppLauncher
exec >/dev/null 2>/dev/null
python3 PfkAppLauncher.py config-linux.ini &

exit 0
