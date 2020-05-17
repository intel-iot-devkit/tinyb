#!/bin/sh

#
# ../scripts/run-java-scanner01.sh -wait -mac C0:26:DA:01:DA:B1 -mode 0 2>&1 | tee ~/scanner-h01-java01.log
#

if [ ! -e lib/java/tinyb2.jar -o ! -e bin/java/ScannerTinyB01.jar -o ! -e lib/libtinyb.so -o ! -e lib/libdirect_bt.so ] ; then
    echo run from dist directory
    exit 1
fi
java -cp lib/java/tinyb2.jar:bin/java/ScannerTinyB01.jar -Djava.library.path=`pwd`/lib ScannerTinyB01 $*
