#!/bin/sh

#
# ../scripts/run-java-scanner00.sh -wait -mac C0:26:DA:01:DA:B1 2>&1 | tee ~/scanner-h01-java00.log
#

if [ ! -e lib/java/tinyb2.jar -o ! -e bin/java/ScannerTinyB00.jar -o ! -e lib/libdirect_bt.so ] ; then
    echo run from dist directory
    exit 1
fi

# run 'dpkg-reconfigure locales' enable 'en_US.UTF-8'
export LANG=en_US.UTF-8
export LC_MEASUREMENT=en_US.UTF-8

java -cp lib/java/tinyb2.jar:bin/java/ScannerTinyB00.jar -Djava.library.path=`pwd`/lib ScannerTinyB00 $*
