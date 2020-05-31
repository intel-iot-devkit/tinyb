#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
logfile=`basename $0 .sh`.log
rm -f $logfile

export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-armhf

doit() {
    echo rootdir $rootdir
    echo logfile $logfile

    cd $rootdir/build-armhf
    make install test
    cp -a examples/* $rootdir/dist-armhf/bin

    cd $rootdir
}

doit 2>&1 | tee $logfile
