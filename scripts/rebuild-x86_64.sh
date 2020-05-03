#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
logfile=`basename $0 .sh`.log
rm -f $logfile

export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64

doit() {
    echo rootdir $rootdir
    echo logfile $logfile

    cd $rootdir/build-x86_64
    make install test
    cp -a examples/* $rootdir/dist-x86_64/bin

    cd $rootdir
}

doit 2>&1 | tee $logfile
