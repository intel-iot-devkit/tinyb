#!/bin/sh

if [ ! -e lib/java/tinyb2.jar -o ! -e bin/java/ScannerTinyB00.jar -o ! -e lib/libtinyb.so -o ! -e lib/libdirect_bt.so ] ; then
    echo run from dist directory
    exit 1
fi
java -cp lib/java/tinyb2.jar:bin/java/ScannerTinyB00.jar -Djava.library.path=`pwd`/lib ScannerTinyB00 $*
