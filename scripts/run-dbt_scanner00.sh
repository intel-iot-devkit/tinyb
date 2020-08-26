#!/bin/sh

#
# ../scripts/run-dbt_scanner00.sh -wait -mac C0:26:DA:01:DA:B1 2>&1 | tee ~/scanner-h01-dbt00.log
#

if [ ! -e bin/dbt_scanner00 -o ! -e lib/libdirect_bt.so ] ; then
    echo run from dist directory
    exit 1
fi
# hciconfig hci0 reset

# run 'dpkg-reconfigure locales' enable 'en_US.UTF-8'
export LANG=en_US.UTF-8
export LC_MEASUREMENT=en_US.UTF-8

#LD_LIBRARY_PATH=`pwd`/lib strace bin/dbt_scanner00 $*
LD_LIBRARY_PATH=`pwd`/lib bin/dbt_scanner00 $*
