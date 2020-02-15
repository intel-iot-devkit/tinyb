#!/bin/sh

if [ ! -e bin/hci_scanner -o ! -e lib/libtinyb.so -o ! -e lib/libtinyb_hci.so ] ; then
    echo run from dist directory
    exit 1
fi
hciconfig hci0 reset
LD_LIBRARY_PATH=`pwd`/lib bin/hci_scanner $*
