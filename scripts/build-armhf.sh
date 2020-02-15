#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
echo rootdir $rootdir

export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-armhf

cd $rootdir
rm -rf dist-armhf
mkdir -p dist-armhf/bin
# rm -rf build-armhf
mkdir build-armhf
cd build-armhf
cmake -DCMAKE_INSTALL_PREFIX=$rootdir/dist-armhf -DBUILDJAVA=ON -DBUILDEXAMPLES=ON ..
make
make install
cp -a examples/* $rootdir/dist-armhf/bin

cd $rootdir
