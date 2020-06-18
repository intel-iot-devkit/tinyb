#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`
bname=`basename $0 .sh`
logfile=$bname.log
rm -f $logfile

. $sdir/setup-machine-arch.sh

export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-$archabi
if [ ! -e $JAVA_HOME ] ; then
    echo $JAVA_HOME does not exist
    exit 1
fi

buildit() {
    echo rootdir $rootdir
    echo logfile $logfile

    cd $rootdir/build-$archabi
    make install test
    if [ $? -eq 0 ] ; then
        echo "REBUILD SUCCESS $bname $archabi"
        cp -a examples/* $rootdir/dist-$archabi/bin
        cd $rootdir
        return 0
    else
        echo "REBUILD FAILURE $bname $archabi"
        cd $rootdir
        return 1
    fi
}

buildit 2>&1 | tee $logfile

