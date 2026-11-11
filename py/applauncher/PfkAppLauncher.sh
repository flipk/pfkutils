#!/bin/bash

pycands=(
    ${HOME}/pfk/${PFKARCH}/venv/bin/python3
    /usr/local/bin/python3
    /bin/python3
    /usr/bin/python3
)

for py in "${pycands[@]}" ; do
    if [[ -x $py ]] ; then
        break
    fi
done

set -e -x

cd ${HOME}/pfk/PfkAppLauncher
exec >/dev/null 2>/dev/null
$py PfkAppLauncher.py config-linux.ini &

exit 0
