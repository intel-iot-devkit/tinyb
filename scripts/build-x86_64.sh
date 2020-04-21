#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
logfile=`basename $0 .sh`.log
rm -f $logfile

export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64

doit() {
    echo rootdir $rootdir
    echo logfile $logfile

    cd $rootdir
    rm -rf dist-x86_64
    mkdir -p dist-x86_64/bin
    rm -rf build-x86_64
    mkdir -p build-x86_64
    cd build-x86_64
    cmake -DCMAKE_INSTALL_PREFIX=$rootdir/dist-x86_64 -DBUILDJAVA=ON -DBUILDEXAMPLES=ON -DBUILD_TESTING=ON ..
    #cmake -DCMAKE_INSTALL_PREFIX=$rootdir/dist-x86_64 -DBUILDJAVA=ON -DBUILDEXAMPLES=ON -DBUILD_TESTING=ON -DDEBUG=ON ..
    make install test
    cp -a examples/* $rootdir/dist-x86_64/bin

    cd $rootdir
}

doit 2>&1 | tee $logfile
