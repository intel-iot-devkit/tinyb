#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
echo rootdir $rootdir

#
# export JAVA_HOME=/opt-linux-x86_64/jdk1.8.0_121
#

cd $rootdir
rm -rf dist-x86_64
mkdir -p dist-x86_64/bin
rm -rf build-x86_64
mkdir build-x86_64
cd build-x86_64
cmake -DCMAKE_INSTALL_PREFIX=$rootdir/dist-x86_64 -DBUILDJAVA=ON -DBUILDEXAMPLES=ON ..
make
make install
cp -a examples/* $rootdir/dist-x86_64/bin

cd $rootdir
