#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
logfile=`basename $0 .sh`.log
rm -f $logfile

export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-armhf

doit() {
    echo rootdir $rootdir
    echo logfile $logfile

    cd $rootdir
    rm -rf dist-armhf
    mkdir -p dist-armhf/bin
    rm -rf build-armhf
    mkdir build-armhf
    cd build-armhf
    cmake -DCMAKE_INSTALL_PREFIX=$rootdir/dist-armhf -DBUILDJAVA=ON -DBUILDEXAMPLES=ON -DBUILD_TESTING=ON ..
    #cmake -DCMAKE_INSTALL_PREFIX=$rootdir/dist-armhf -DBUILDJAVA=ON -DBUILDEXAMPLES=ON -DBUILD_TESTING=ON -DDEBUG=ON ..
    make
    make test
    make install
    cp -a examples/* $rootdir/dist-armhf/bin

    cd $rootdir
}

doit 2>&1 | tee $logfile
