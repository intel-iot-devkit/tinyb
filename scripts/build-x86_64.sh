#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
echo rootdir $rootdir

export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64

cd $rootdir
rm -rf dist-x86_64
mkdir -p dist-x86_64/bin
rm -rf build-x86_64
mkdir build-x86_64
cd build-x86_64
# cmake -DCMAKE_INSTALL_PREFIX=$rootdir/dist-x86_64 -DBUILDJAVA=ON -DBUILDEXAMPLES=ON -DBUILD_TESTING=ON ..
cmake -DCMAKE_INSTALL_PREFIX=$rootdir/dist-x86_64 -DBUILDEXAMPLES=ON -DDEBUG=ON -DBUILD_TESTING=ON ..
make
make test
make install
cp -a examples/* $rootdir/dist-x86_64/bin

cd $rootdir
