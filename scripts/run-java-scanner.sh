#!/bin/sh

if [ ! -e java/tinyb2.jar -o ! -e examples/java/ScannerTinyB.jar ] ; then
    echo run from build directory
    exit 1
fi
java -cp java/tinyb2.jar:examples/java/ScannerTinyB.jar -Djava.library.path=`pwd`/java/jni:`pwd`/src ScannerTinyB $*
