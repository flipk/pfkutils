#!/bin/bash

pfk_environ_key=${HOME}/pfk/etc/pfk_environ_key

if [[ ! -f $pfk_environ_key ]]; then
    echo ERROR: no $pfk_environ_key 1>&2;
    return 1;
fi;
pfk_envfile=;
if [[ -f .pfk_environ ]]; then
    pfk_envfile=.pfk_environ;
else
    if [[ -f 0-pfk_environ ]]; then
        pfk_envfile=0-pfk_environ;
    else
        echo ERROR: no .pfk_environ in this directory 1>&2;
        return 1;
    fi;
fi;
echo signing $PWD/${pfk_envfile} ...;
grep -v '#pfk_sign' ${pfk_envfile} > .pfk_environ.1;
set -- `( cat $pfk_environ_key .pfk_environ.1 ; echo $PWD ) | sha256sum -`;
echo '#pfk_sign' $1 >> .pfk_environ.1;
mv .pfk_environ.1 ${pfk_envfile}

exit 0
