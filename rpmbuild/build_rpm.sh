#!/bin/bash

cd ${0%/build_rpm.sh} || {
    echo this script should be called build_rpm.sh
    echo 0 = $0
    exit 1
}
rpmbuild=$PWD
cd ..
export PFKUTILS=$PWD
export ORIG_HOME=$HOME
export HOME=$PFKUTILS
export PATH=/usr/local/bin:/bin:/usr/bin:/sbin:/usr/sbin:/usr/local/sbin
export OBJDIR=obj.leviathan
echo PFKUTILS = $PFKUTILS
echo HOME = $HOME
echo rpmbuild = $rpmbuild

gitversion=$( git describe --match 'v*.*' )
version=$( echo $gitversion | sed -e 's,^v\([0-9]*\.[0-9]*\).*$,\1,')
build_num=1

cd $rpmbuild

rm -rf .rpmdb BUILD BUILDROOT root RPMS SRPMS SPECS/pfkutils.spec
sed \
	-e "s,@@RPMBUILD@@,${rpmbuild},g" \
	-e "s,@@VERSION@@,$version,g" \
	-e "s,@@BUILD_NUM@@,$build_num,g" \
        -e "s,@@ORIG_HOME@@,$ORIG_HOME,g" \
	< SPECS/pfkutils-template.spec > SPECS/pfkutils.spec
rpmbuild -bb SPECS/pfkutils.spec
rpmbuild_exit=$?
if [[ $rpmbuild_exit -ne 0 ]] ; then
    echo RPMBUILD FAILED, STOPPING HERE
    exit $rpmbuild_exit
fi

rm -rf .rpmdb BUILD BUILDROOT root SRPMS SPECS/pfkutils.spec

ls -l $( find . -name '*.rpm' )

exit 0
