#!/bin/sh

#
# ../scripts/run-dbt_scanner.sh -wait -mac C0:26:DA:01:DA:B1 2>&1 | tee ~/dbt_scanner.log
#

if [ ! -e bin/dbt_scanner -o ! -e lib/libtinyb.so -o ! -e lib/libdirect_bt.so ] ; then
    echo run from dist directory
    exit 1
fi
# hciconfig hci0 reset
#LD_LIBRARY_PATH=`pwd`/lib strace bin/dbt_scanner $*
LD_LIBRARY_PATH=`pwd`/lib bin/dbt_scanner $*
