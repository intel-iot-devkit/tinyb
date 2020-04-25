#! /bin/sh

sdir=`dirname $(readlink -f $0)`
rootdir=`dirname $sdir`

clear ; clang++ -c -o a.o -std=c++11 -Wall -I$rootdir/api/direct_bt -I$rootdir/ieee11073 -I$rootdir/api $1 2>&1 | tee make.log
rm -f a.o
