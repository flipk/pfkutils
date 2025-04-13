#!/bin/bash

version=3.13.3
short_vers=3.13
packages="virtualenv pipreqs \
     protobuf mypy-protobuf types-protobuf python-protobuf \
     numpy pandas scipy \
     websockets matplotlib"

# umap python-prctl

build_path=/vice2/python/build-$short_vers
install_path=/vice2/python/$short_vers
dnload_path=/vice2/python/dnload-$short_vers

srctar=Python-${version}.tgz
srcdir=Python-${version}
srcurl=https://www.python.org/ftp/python/${version}/${srctar}

cpus=$( grep ^processor /proc/cpuinfo | wc -l )
makejobs=-j$cpus

mkdir -p $build_path $dnload_path
cd $build_path

if [[ ! -f $srctar ]] ; then
    wget $srcurl
fi

if [[ ! -d $srcdir ]] ; then
    tar zxf $srctar
fi

cd $srcdir
./configure --prefix=$install_path --disable-test-modules
make $makejobs
make install

export PATH=${install_path}/bin:$PATH

pip3 install --upgrade pip
pip3 install $packages
#pip3 download -d $dnload_path $packages

exit 0
